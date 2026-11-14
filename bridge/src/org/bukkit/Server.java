package org.bukkit;

import java.util.Collection;
import java.util.List;
import java.util.UUID;
import java.util.logging.Logger;
import net.kyori.adventure.text.Component;
import org.bukkit.command.CommandSender;
import org.bukkit.command.ConsoleCommandSender;
import org.bukkit.command.PluginCommand;
import org.bukkit.entity.Player;
import org.bukkit.plugin.PluginManager;
import org.bukkit.scheduler.BukkitScheduler;

public interface Server {
    String getName();

    String getVersion();

    String getBukkitVersion();

    String getMinecraftVersion();

    Collection<? extends Player> getOnlinePlayers();

    int getMaxPlayers();

    int getPort();

    String getIp();

    String getMotd();

    boolean getOnlineMode();

    int getViewDistance();

    GameMode getDefaultGameMode();

    Player getPlayer(String name);

    Player getPlayerExact(String name);

    Player getPlayer(UUID id);

    List<Player> matchPlayer(String name);

    OfflinePlayer getOfflinePlayer(String name);

    OfflinePlayer getOfflinePlayer(UUID id);

    PluginManager getPluginManager();

    BukkitScheduler getScheduler();

    World getWorld(String name);

    World getWorld(UUID uid);

    List<World> getWorlds();

    int broadcastMessage(String message);

    int broadcast(String message, String permission);

    int broadcast(Component message);

    ConsoleCommandSender getConsoleSender();

    boolean dispatchCommand(CommandSender sender, String commandLine);

    PluginCommand getPluginCommand(String name);

    Logger getLogger();

    boolean isPrimaryThread();

    void shutdown();

    double[] getTPS();

    void reload();

    void savePlayers();

    // ---- Inventare, Bossbars, Scoreboards ----
    org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, int size);

    org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, int size, String title);

    default org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, int size, net.kyori.adventure.text.Component title) {
        return createInventory(owner, size, title == null ? null : title.toLegacy());
    }

    org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, org.bukkit.event.inventory.InventoryType type);

    org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, org.bukkit.event.inventory.InventoryType type, String title);

    default org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, org.bukkit.event.inventory.InventoryType type,
                                                           net.kyori.adventure.text.Component title) {
        return createInventory(owner, type, title == null ? null : title.toLegacy());
    }

    org.bukkit.boss.BossBar createBossBar(String title, org.bukkit.boss.BarColor color, org.bukkit.boss.BarStyle style, org.bukkit.boss.BarFlag... flags);

    org.bukkit.scoreboard.ScoreboardManager getScoreboardManager();

    default org.bukkit.inventory.ItemFactory getItemFactory() { return org.bukkit.inventory.ItemFactory.INSTANCE; }
}
