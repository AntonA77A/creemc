package org.bukkit.scheduler;

import java.util.concurrent.Callable;
import java.util.concurrent.Future;
import java.util.function.Consumer;
import org.bukkit.plugin.Plugin;

public interface BukkitScheduler {
    BukkitTask runTask(Plugin plugin, Runnable task);

    void runTask(Plugin plugin, Consumer<? super BukkitTask> task);

    BukkitTask runTaskLater(Plugin plugin, Runnable task, long delay);

    void runTaskLater(Plugin plugin, Consumer<? super BukkitTask> task, long delay);

    BukkitTask runTaskTimer(Plugin plugin, Runnable task, long delay, long period);

    void runTaskTimer(Plugin plugin, Consumer<? super BukkitTask> task, long delay, long period);

    BukkitTask runTaskAsynchronously(Plugin plugin, Runnable task);

    void runTaskAsynchronously(Plugin plugin, Consumer<? super BukkitTask> task);

    BukkitTask runTaskLaterAsynchronously(Plugin plugin, Runnable task, long delay);

    BukkitTask runTaskTimerAsynchronously(Plugin plugin, Runnable task, long delay, long period);

    int scheduleSyncDelayedTask(Plugin plugin, Runnable task, long delay);

    int scheduleSyncDelayedTask(Plugin plugin, Runnable task);

    int scheduleSyncRepeatingTask(Plugin plugin, Runnable task, long delay, long period);

    <T> Future<T> callSyncMethod(Plugin plugin, Callable<T> task);

    void cancelTask(int taskId);

    void cancelTasks(Plugin plugin);

    boolean isCurrentlyRunning(int taskId);

    boolean isQueued(int taskId);
}
