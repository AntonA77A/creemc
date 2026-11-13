package io.papermc.paper.command.brigadier;

import java.util.Collection;
import java.util.List;

/** Paper: einfacher Befehl ohne Brigadier-Baum (JavaPlugin#registerCommand). */
public interface BasicCommand {
    void execute(CommandSourceStack source, String[] args);

    default Collection<String> suggest(CommandSourceStack source, String[] args) { return List.of(); }

    default boolean canUse(org.bukkit.command.CommandSender sender) {
        String p = permission();
        return p == null || sender.hasPermission(p);
    }

    default String permission() { return null; }
}
