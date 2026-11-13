package org.bukkit;

import java.util.UUID;
import org.bukkit.entity.Player;
import org.bukkit.permissions.ServerOperator;

public interface OfflinePlayer extends ServerOperator {
    boolean isOnline();

    String getName();

    UUID getUniqueId();

    Player getPlayer();

    boolean hasPlayedBefore();
}
