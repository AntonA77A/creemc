package creemc.bridge;

import java.util.logging.Level;
import java.util.logging.Logger;
import net.kyori.adventure.text.Component;
import net.kyori.adventure.text.logger.slf4j.ComponentLogger;

/** SLF4J-/ComponentLogger auf java.util.logging. */
public final class Slf4j implements ComponentLogger {
    private final Logger log;

    public Slf4j(Logger log) { this.log = log; }

    private static String fmt(String format, Object... args) {
        if (format == null) return "null";
        StringBuilder b = new StringBuilder();
        int a = 0, i = 0;
        while (i < format.length()) {
            if (format.startsWith("{}", i) && a < args.length) {
                b.append(args[a++]);
                i += 2;
            } else b.append(format.charAt(i++));
        }
        return b.toString();
    }

    private void log(Level l, String format, Object... args) {
        Throwable t = args.length > 0 && args[args.length - 1] instanceof Throwable th ? th : null;
        log.log(l, fmt(format, args), t);
    }

    @Override public String getName() { return log.getName(); }
    @Override public boolean isDebugEnabled() { return false; }
    @Override public boolean isInfoEnabled() { return true; }
    @Override public void trace(String msg) { }
    @Override public void trace(String format, Object... args) { }
    @Override public void debug(String msg) { log.fine(msg); }
    @Override public void debug(String format, Object arg) { log(Level.FINE, format, arg); }
    @Override public void debug(String format, Object... args) { log(Level.FINE, format, args); }
    @Override public void debug(String msg, Throwable t) { log.log(Level.FINE, msg, t); }
    @Override public void info(String msg) { log.info(msg); }
    @Override public void info(String format, Object arg) { log(Level.INFO, format, arg); }
    @Override public void info(String format, Object arg1, Object arg2) { log(Level.INFO, format, arg1, arg2); }
    @Override public void info(String format, Object... args) { log(Level.INFO, format, args); }
    @Override public void info(String msg, Throwable t) { log.log(Level.INFO, msg, t); }
    @Override public void warn(String msg) { log.warning(msg); }
    @Override public void warn(String format, Object arg) { log(Level.WARNING, format, arg); }
    @Override public void warn(String format, Object arg1, Object arg2) { log(Level.WARNING, format, arg1, arg2); }
    @Override public void warn(String format, Object... args) { log(Level.WARNING, format, args); }
    @Override public void warn(String msg, Throwable t) { log.log(Level.WARNING, msg, t); }
    @Override public void error(String msg) { log.severe(msg); }
    @Override public void error(String format, Object arg) { log(Level.SEVERE, format, arg); }
    @Override public void error(String format, Object arg1, Object arg2) { log(Level.SEVERE, format, arg1, arg2); }
    @Override public void error(String format, Object... args) { log(Level.SEVERE, format, args); }
    @Override public void error(String msg, Throwable t) { log.log(Level.SEVERE, msg, t); }
    @Override public void info(Component msg) { log.info(msg.toLegacy()); }
    @Override public void warn(Component msg) { log.warning(msg.toLegacy()); }
    @Override public void error(Component msg) { log.severe(msg.toLegacy()); }
}
