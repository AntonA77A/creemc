package creemc.bridge;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Consumer;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.bukkit.plugin.Plugin;
import org.bukkit.scheduler.BukkitScheduler;
import org.bukkit.scheduler.BukkitTask;

/** Bukkit-Scheduler: synchrone Aufgaben laufen im Server-Tick (20/s), asynchrone in einem Thread-Pool. */
public final class Scheduler implements BukkitScheduler {
    private static final Logger LOG = Logger.getLogger("CreeMC-Bridge");
    private final Bridge bridge;
    private final AtomicInteger ids = new AtomicInteger();
    private final List<Task> sync = new ArrayList<>();
    private final List<Task> async = new ArrayList<>();
    private final ScheduledExecutorService pool = Executors.newScheduledThreadPool(4, r -> {
        Thread t = new Thread(r, "CreeMC-Bridge-Async");
        t.setDaemon(true);
        return t;
    });

    Scheduler(Bridge bridge) { this.bridge = bridge; }

    final class Task implements BukkitTask {
        final int id = ids.incrementAndGet();
        final Plugin owner;
        final Runnable run;
        final boolean isSync;
        final long period;
        long next;
        volatile boolean cancelled;
        volatile boolean running;
        ScheduledFuture<?> future;

        Task(Plugin owner, Runnable run, boolean isSync, long delay, long period) {
            this.owner = owner;
            this.run = run;
            this.isSync = isSync;
            this.period = period;
            this.next = bridge.tick + Math.max(isSync ? 1 : 0, delay);
        }

        @Override public int getTaskId() { return id; }
        @Override public Plugin getOwner() { return owner; }
        @Override public boolean isSync() { return isSync; }
        @Override public boolean isCancelled() { return cancelled; }

        @Override
        public void cancel() {
            cancelled = true;
            if (future != null) future.cancel(false);
        }

        void execute() {
            running = true;
            try {
                run.run();
            } catch (Throwable t) {
                LOG.log(Level.WARNING, "Task #" + id + " for " + owner.getDescription().getFullName() + " generated an exception", t);
            } finally {
                running = false;
            }
        }
    }

    private Task schedule(Plugin plugin, Runnable r, boolean isSync, long delay, long period) {
        if (plugin == null) throw new IllegalArgumentException("Plugin cannot be null");
        if (!plugin.isEnabled()) throw new IllegalStateException("Plugin attempted to register task while disabled");
        Task t = new Task(plugin, r, isSync, delay, period);
        if (isSync) {
            synchronized (sync) {
                sync.add(t);
            }
        } else {
            synchronized (async) {
                async.add(t);
            }
            long ms = Math.max(0, delay) * 50;
            t.future = period > 0 ? pool.scheduleAtFixedRate(t::execute, ms, Math.max(1, period) * 50, TimeUnit.MILLISECONDS)
                    : pool.schedule(() -> {
                t.execute();
                t.cancelled = true;
            }, ms, TimeUnit.MILLISECONDS);
        }
        return t;
    }

    /** Ein Server-Tick: fällige synchrone Aufgaben ausführen. */
    void runTick() {
        List<Task> due = new ArrayList<>();
        synchronized (sync) {
            for (Iterator<Task> it = sync.iterator(); it.hasNext(); ) {
                Task t = it.next();
                if (t.cancelled) it.remove();
                else if (t.next <= bridge.tick) due.add(t);
            }
        }
        for (Task t : due) {
            if (t.cancelled) continue;
            t.execute();
            if (t.period > 0) t.next = bridge.tick + Math.max(1, t.period);
            else t.cancelled = true;
        }
        synchronized (async) {
            async.removeIf(t -> t.cancelled);
        }
    }

    void shutdown() { pool.shutdownNow(); }

    private Runnable wrap(Consumer<? super BukkitTask> c, BukkitTask[] holder) { return () -> c.accept(holder[0]); }

    @Override public BukkitTask runTask(Plugin p, Runnable r) { return schedule(p, r, true, 0, 0); }

    @Override
    public void runTask(Plugin p, Consumer<? super BukkitTask> c) {
        BukkitTask[] h = new BukkitTask[1];
        h[0] = schedule(p, wrap(c, h), true, 0, 0);
    }

    @Override public BukkitTask runTaskLater(Plugin p, Runnable r, long delay) { return schedule(p, r, true, delay, 0); }

    @Override
    public void runTaskLater(Plugin p, Consumer<? super BukkitTask> c, long delay) {
        BukkitTask[] h = new BukkitTask[1];
        h[0] = schedule(p, wrap(c, h), true, delay, 0);
    }

    @Override public BukkitTask runTaskTimer(Plugin p, Runnable r, long delay, long period) { return schedule(p, r, true, delay, Math.max(1, period)); }

    @Override
    public void runTaskTimer(Plugin p, Consumer<? super BukkitTask> c, long delay, long period) {
        BukkitTask[] h = new BukkitTask[1];
        h[0] = schedule(p, wrap(c, h), true, delay, Math.max(1, period));
    }

    @Override public BukkitTask runTaskAsynchronously(Plugin p, Runnable r) { return schedule(p, r, false, 0, 0); }

    @Override
    public void runTaskAsynchronously(Plugin p, Consumer<? super BukkitTask> c) {
        BukkitTask[] h = new BukkitTask[1];
        h[0] = schedule(p, wrap(c, h), false, 0, 0);
    }

    @Override public BukkitTask runTaskLaterAsynchronously(Plugin p, Runnable r, long delay) { return schedule(p, r, false, delay, 0); }

    @Override
    public BukkitTask runTaskTimerAsynchronously(Plugin p, Runnable r, long delay, long period) {
        return schedule(p, r, false, delay, Math.max(1, period));
    }

    @Override public int scheduleSyncDelayedTask(Plugin p, Runnable r, long delay) { return runTaskLater(p, r, delay).getTaskId(); }

    @Override public int scheduleSyncDelayedTask(Plugin p, Runnable r) { return runTask(p, r).getTaskId(); }

    @Override public int scheduleSyncRepeatingTask(Plugin p, Runnable r, long delay, long period) { return runTaskTimer(p, r, delay, period).getTaskId(); }

    @Override
    public <T> Future<T> callSyncMethod(Plugin p, Callable<T> task) {
        CompletableFuture<T> f = new CompletableFuture<>();
        runTask(p, () -> {
            try {
                f.complete(task.call());
            } catch (Throwable t) {
                f.completeExceptionally(t);
            }
        });
        return f;
    }

    private List<Task> allTasks() {
        List<Task> out = new ArrayList<>();
        synchronized (sync) {
            out.addAll(sync);
        }
        synchronized (async) {
            out.addAll(async);
        }
        return out;
    }

    @Override
    public void cancelTask(int taskId) {
        for (Task t : allTasks()) if (t.id == taskId) t.cancel();
    }

    @Override
    public void cancelTasks(Plugin plugin) {
        for (Task t : allTasks()) if (t.owner == plugin) t.cancel();
    }

    @Override
    public boolean isCurrentlyRunning(int taskId) {
        for (Task t : allTasks()) if (t.id == taskId) return t.running;
        return false;
    }

    @Override
    public boolean isQueued(int taskId) {
        for (Task t : allTasks()) if (t.id == taskId) return !t.cancelled;
        return false;
    }
}
