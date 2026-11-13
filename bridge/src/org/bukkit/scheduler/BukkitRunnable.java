package org.bukkit.scheduler;

import org.bukkit.Bukkit;
import org.bukkit.plugin.Plugin;

public abstract class BukkitRunnable implements Runnable {
    private BukkitTask task;

    public synchronized boolean isCancelled() {
        if (task == null) throw new IllegalStateException("Not scheduled yet");
        return task.isCancelled();
    }

    public synchronized void cancel() {
        if (task == null) throw new IllegalStateException("Not scheduled yet");
        task.cancel();
    }

    public synchronized BukkitTask runTask(Plugin plugin) { return setup(Bukkit.getScheduler().runTask(plugin, (Runnable) this)); }

    public synchronized BukkitTask runTaskAsynchronously(Plugin plugin) {
        return setup(Bukkit.getScheduler().runTaskAsynchronously(plugin, (Runnable) this));
    }

    public synchronized BukkitTask runTaskLater(Plugin plugin, long delay) {
        return setup(Bukkit.getScheduler().runTaskLater(plugin, (Runnable) this, delay));
    }

    public synchronized BukkitTask runTaskLaterAsynchronously(Plugin plugin, long delay) {
        return setup(Bukkit.getScheduler().runTaskLaterAsynchronously(plugin, this, delay));
    }

    public synchronized BukkitTask runTaskTimer(Plugin plugin, long delay, long period) {
        return setup(Bukkit.getScheduler().runTaskTimer(plugin, (Runnable) this, delay, period));
    }

    public synchronized BukkitTask runTaskTimerAsynchronously(Plugin plugin, long delay, long period) {
        return setup(Bukkit.getScheduler().runTaskTimerAsynchronously(plugin, this, delay, period));
    }

    public synchronized int getTaskId() {
        if (task == null) throw new IllegalStateException("Not scheduled yet");
        return task.getTaskId();
    }

    private BukkitTask setup(BukkitTask t) {
        if (task != null) throw new IllegalStateException("Already scheduled as " + task.getTaskId());
        task = t;
        return t;
    }
}
