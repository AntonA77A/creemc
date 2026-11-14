package creemc.api;

import java.util.Collection;
import java.util.List;
import java.util.function.BiConsumer;
import java.util.function.BiFunction;
import java.util.function.Consumer;
import org.bukkit.command.CommandSender;
import org.bukkit.event.Event;
import org.bukkit.event.EventPriority;
import org.bukkit.event.Listener;
import org.bukkit.plugin.java.JavaPlugin;
import org.bukkit.scheduler.BukkitTask;

/**
 * Basisklasse für eigene CreeMC-Plugins: kurz und ohne viel Drumherum.
 * Statt plugin.yml reicht eine creemc.yml (name, version, main) - plugin.yml geht aber auch.
 *
 * <pre>
 * public class MeinPlugin extends CreeMCPlugin {
 *     public void onEnable() {
 *         command("hallo", "Sagt Hallo", (sender, args) -&gt; msg(sender, "&amp;aHallo!"));
 *         on(PlayerJoinEvent.class, e -&gt; msg(e.getPlayer(), "&amp;eWillkommen!"));
 *         every(20 * 60, () -&gt; broadcast("&amp;eEine Minute ist um"));
 *     }
 * }
 * </pre>
 *
 * Alles aus {@link JavaPlugin} (getConfig, saveDefaultConfig, getLogger, getDataFolder ...) geht weiterhin.
 */
public abstract class CreeMCPlugin extends JavaPlugin implements Listener {

    /** Befehl ohne Eintrag in einer yml-Datei. */
    protected void command(String name, String description, BiConsumer<CommandSender, String[]> handler) {
        command(name, description, false, handler, null);
    }

    /** Befehl, optional nur für OPs. */
    protected void command(String name, String description, boolean opOnly, BiConsumer<CommandSender, String[]> handler) {
        command(name, description, opOnly, handler, null);
    }

    /**
     * Befehl mit Tab-Vervollständigung: complete(sender, args) liefert Vorschläge für das letzte Argument.
     */
    protected void command(String name, String description, boolean opOnly, BiConsumer<CommandSender, String[]> handler,
                           BiFunction<CommandSender, String[], Collection<String>> complete) {
        registerCommand(name, description, List.of(), new io.papermc.paper.command.brigadier.BasicCommand() {
            @Override
            public void execute(io.papermc.paper.command.brigadier.CommandSourceStack stack, String[] args) {
                CommandSender s = stack.getSender();
                if (opOnly && !s.isOp()) {
                    s.sendMessage("§cDazu hast du keine Rechte.");
                    return;
                }
                try {
                    handler.accept(s, args);
                } catch (IllegalArgumentException e) {   // bequem: throw new IllegalArgumentException("Benutzung: ...")
                    s.sendMessage("§c" + e.getMessage());
                }
            }

            @Override
            public Collection<String> suggest(io.papermc.paper.command.brigadier.CommandSourceStack stack, String[] args) {
                return complete == null ? List.of() : complete.apply(stack.getSender(), args);
            }
        });
    }

    /** Auf ein Bukkit-Event hören: on(PlayerJoinEvent.class, e -&gt; ...). */
    protected <T extends Event> void on(Class<T> event, Consumer<T> handler) { on(event, EventPriority.NORMAL, handler); }

    /** Auf ein Event hören, mit Priorität (LOWEST zuerst ... MONITOR zuletzt). */
    protected <T extends Event> void on(Class<T> event, EventPriority priority, Consumer<T> handler) {
        getServer().getPluginManager().registerEvent(event, this, priority, (l, e) -> {
            if (event.isInstance(e)) handler.accept(event.cast(e));
        }, this);
    }

    /** Einmal nach 'ticks' Ticks ausführen (20 Ticks = 1 Sekunde). */
    protected BukkitTask later(long ticks, Runnable task) { return getServer().getScheduler().runTaskLater(this, task, ticks); }

    /** Alle 'ticks' Ticks ausführen. */
    protected BukkitTask every(long ticks, Runnable task) { return getServer().getScheduler().runTaskTimer(this, task, ticks, ticks); }

    /** Nachricht mit &amp;-Farbcodes an einen Spieler oder die Konsole. */
    protected static void msg(CommandSender to, String message) { to.sendMessage(CreeMC.color(message)); }

    /** Text mit &amp;-Farbcodes an alle. */
    protected static void broadcast(String message) { CreeMC.broadcast(message); }
}
