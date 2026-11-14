package org.bukkit.boss;

import org.bukkit.NamespacedKey;

public interface KeyedBossBar extends BossBar {
    NamespacedKey getKey();
}