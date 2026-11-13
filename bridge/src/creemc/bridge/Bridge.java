package creemc.bridge;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import org.bukkit.Bukkit;
import org.bukkit.plugin.Plugin;

/**
 * Brücke zwischen dem Python-Server (CreeMC) und Paper-/Bukkit-Plugins.
 * Protokoll: eine JSON-Nachricht pro Zeile über stdin/stdout (siehe creemc/paper.py).
 */
public final class Bridge {
    private static Bridge instance;

    public final List<Plugin> plugins = new ArrayList<>();
    public final Commands commands = new Commands(this);
    public final Events events = new Events(this);
    public final Scheduler scheduler = new Scheduler(this);
    public ServerImpl server;
    public Thread mainThread;
    public volatile long tick;

    private final PrintStream out;
    private final LinkedBlockingQueue<Map<String, Object>> inbox = new LinkedBlockingQueue<>();
    private final Map<Long, CompletableFuture<Object>> pending = new ConcurrentHashMap<>();
    private final AtomicLong ids = new AtomicLong();
    private volatile boolean running = true;

    private Bridge(PrintStream out) { this.out = out; }

    public static Bridge get() { return instance; }

    public static void main(String[] args) throws Exception {
        // stdout gehört dem Protokoll: alles, was Plugins mit System.out ausgeben, wird zur Log-Zeile
        PrintStream protocol = new PrintStream(new FileOutputStream(java.io.FileDescriptor.out), true, StandardCharsets.UTF_8);
        System.setOut(new PrintStream(new LineLogger("STDOUT"), true, StandardCharsets.UTF_8));
        setupLogging();
        instance = new Bridge(protocol);
        instance.run(new File(args.length > 0 ? args[0] : "plugins"));
    }

    // ---- Logging: jede Zeile geht über stderr an Python ("LEVEL<TAB>Name<TAB>Text") ----

    private static void setupLogging() {
        Logger root = Logger.getLogger("");
        for (Handler h : root.getHandlers()) root.removeHandler(h);
        root.addHandler(new Handler() {
            @Override
            public void publish(LogRecord r) {
                String msg = r.getMessage();
                if (r.getParameters() != null && msg != null && msg.contains("{0")) {
                    try {
                        msg = java.text.MessageFormat.format(msg, r.getParameters());
                    } catch (IllegalArgumentException ignored) {
                    }
                }
                StringBuilder b = new StringBuilder(msg == null ? "" : msg);
                if (r.getThrown() != null) {
                    java.io.StringWriter sw = new java.io.StringWriter();
                    r.getThrown().printStackTrace(new java.io.PrintWriter(sw));
                    b.append('\n').append(sw);
                }
                String name = r.getLoggerName() == null ? "" : r.getLoggerName();
                String lvl = r.getLevel().intValue() >= Level.SEVERE.intValue() ? "ERROR"
                        : r.getLevel().intValue() >= Level.WARNING.intValue() ? "WARNING"
                        : r.getLevel().intValue() >= Level.INFO.intValue() ? "INFO" : "DEBUG";
                synchronized (System.err) {
                    for (String line : b.toString().split("\n")) System.err.println(lvl + "\t" + name + "\t" + line.stripTrailing());
                    System.err.flush();
                }
            }

            @Override
            public void flush() { System.err.flush(); }

            @Override
            public void close() { }
        });
        root.setLevel(Level.INFO);
    }

    public static Logger pluginLogger(String name) {
        Logger l = Logger.getLogger(name);
        l.setParent(Logger.getLogger(""));
        return l;
    }

    private static final class LineLogger extends OutputStream {
        private final Logger log;
        private final java.io.ByteArrayOutputStream buf = new java.io.ByteArrayOutputStream();

        LineLogger(String name) { log = Logger.getLogger(name); }

        @Override
        public synchronized void write(int b) {
            if (b == '\n') {
                log.info(buf.toString(StandardCharsets.UTF_8).stripTrailing());
                buf.reset();
            } else buf.write(b);
        }
    }

    // ---- Senden ----

    public void send(Map<String, Object> msg) {
        String line = Json.write(msg);
        synchronized (out) {
            out.println(line);
            out.flush();
        }
    }

    /** Auftrag an den Server ohne Antwort (Nachricht senden, teleportieren ...). */
    public void call(String op, Object... kv) {
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("t", "call");
        m.put("op", op);
        for (int i = 0; i + 1 < kv.length; i += 2) m.put((String) kv[i], kv[i + 1]);
        send(m);
    }

    /** Frage an den Server, wartet auf die Antwort (z.B. welcher Block an x,y,z steht). */
    public Object request(String op, Object... kv) {
        long id = ids.incrementAndGet();
        CompletableFuture<Object> f = new CompletableFuture<>();
        pending.put(id, f);
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("t", "req");
        m.put("id", id);
        m.put("op", op);
        for (int i = 0; i + 1 < kv.length; i += 2) m.put((String) kv[i], kv[i + 1]);
        send(m);
        try {
            return f.get(10, TimeUnit.SECONDS);
        } catch (Exception e) {
            Logger.getLogger("CreeMC-Bridge").warning("Keine Antwort vom Server auf '" + op + "'");
            return null;
        } finally {
            pending.remove(id);
        }
    }

    // ---- Hauptschleife ----

    private void run(File pluginDir) throws Exception {
        mainThread = Thread.currentThread();
        Thread reader = new Thread(this::readLoop, "CreeMC-Bridge-Reader");
        reader.setDaemon(true);
        reader.start();

        Map<String, Object> init = inbox.take();   // erste Nachricht: Serverinfos
        server = new ServerImpl(this, Json.obj(init.get("server")));
        Bukkit.setServer(server);
        for (Object p : Json.list(init.get("players"))) server.updatePlayer(Json.obj(p));

        new PluginLoader(this).loadAll(pluginDir);
        Map<String, Object> ready = new LinkedHashMap<>();
        ready.put("t", "ready");
        ready.put("plugins", plugins.stream().map(p -> p.getDescription().getFullName()).toList());
        ready.put("commands", commands.describe());
        ready.put("events", events.forwardedNames());
        send(ready);

        while (running) {
            Map<String, Object> msg = inbox.poll(1, TimeUnit.SECONDS);
            if (msg == null) continue;
            try {
                handle(msg);
            } catch (Throwable t) {
                Logger.getLogger("CreeMC-Bridge").log(Level.SEVERE, "Fehler bei Nachricht " + msg.get("t"), t);
                if (msg.get("id") != null) reply(msg, Map.of());
            }
        }
    }

    private void readLoop() {
        try (BufferedReader in = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8))) {
            String line;
            while ((line = in.readLine()) != null) {
                if (line.isBlank()) continue;
                Map<String, Object> msg = Json.obj(Json.parse(line));
                if ("reply".equals(msg.get("t"))) {
                    CompletableFuture<Object> f = pending.get((long) Json.num(msg.get("id"), -1));
                    if (f != null) f.complete(msg.get("value"));
                } else inbox.add(msg);
            }
        } catch (Exception e) {
            Logger.getLogger("CreeMC-Bridge").log(Level.SEVERE, "Verbindung zum Server verloren", e);
        }
        // Server weg: sauber beenden
        inbox.add(Map.of("t", "stop"));
    }

    private void reply(Map<String, Object> msg, Map<String, Object> body) {
        Map<String, Object> r = new LinkedHashMap<>(body);
        r.put("t", "result");
        r.put("id", msg.get("id"));
        send(r);
    }

    private void handle(Map<String, Object> msg) {
        switch (Json.str(msg.get("t"), "")) {
            case "tick" -> {
                tick = (long) Json.num(msg.get("tick"), tick + 1);
                if (msg.containsKey("players")) server.syncPlayers(Json.list(msg.get("players")));
                if (msg.containsKey("time")) server.world.time = (long) Json.num(msg.get("time"), 0);
                scheduler.runTick();
            }
            case "event" -> reply(msg, events.fromServer(Json.str(msg.get("name"), ""), Json.obj(msg.get("data"))));
            case "command" -> reply(msg, commands.execute(msg));
            case "complete" -> reply(msg, Map.of("list", commands.complete(msg)));
            case "stop" -> {
                for (int i = plugins.size() - 1; i >= 0; i--) {
                    Plugin p = plugins.get(i);
                    try {
                        if (p instanceof org.bukkit.plugin.java.JavaPlugin jp) jp.setEnabled(false);
                    } catch (Throwable t) {
                        Logger.getLogger("CreeMC-Bridge").log(Level.SEVERE, "Fehler beim Deaktivieren von " + p.getName(), t);
                    }
                }
                scheduler.shutdown();
                send(Map.of("t", "stopped"));
                running = false;
                System.exit(0);
            }
            default -> Logger.getLogger("CreeMC-Bridge").warning("Unbekannte Nachricht: " + msg.get("t"));
        }
    }

    /** Läuft gerade der Hauptthread (wie Bukkits Server-Thread)? */
    public boolean isMainThread() { return Thread.currentThread() == mainThread; }
}
