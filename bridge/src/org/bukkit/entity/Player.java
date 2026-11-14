package org.bukkit.entity;

import java.net.InetSocketAddress;
import net.kyori.adventure.text.Component;
import org.bukkit.Location;
import org.bukkit.OfflinePlayer;

public interface Player extends HumanEntity, OfflinePlayer {
    String getDisplayName();

    void setDisplayName(String name);

    String getPlayerListName();

    void setPlayerListName(String name);

    default Component displayName() { return Component.text(getDisplayName()); }

    default void displayName(Component name) { setDisplayName(name == null ? null : name.toLegacy()); }

    default void playerListName(Component name) { setPlayerListName(name == null ? null : name.toLegacy()); }

    void kickPlayer(String message);

    default void kick(Component message) { kickPlayer(message == null ? "" : message.toLegacy()); }

    default void kick() { kickPlayer("Kicked"); }

    void chat(String message);

    boolean performCommand(String command);

    boolean isSneaking();

    boolean isSprinting();

    boolean isFlying();

    int getFoodLevel();

    void setFoodLevel(int value);

    float getSaturation();

    void setSaturation(float value);

    int getLevel();

    void setLevel(int level);

    float getExp();

    void setExp(float exp);

    int getTotalExperience();

    void setTotalExperience(int exp);

    void giveExp(int amount);

    void giveExpLevels(int amount);

    void sendTitle(String title, String subtitle);

    void sendTitle(String title, String subtitle, int fadeIn, int stay, int fadeOut);

    void sendActionBar(String message);

    @Override
    default void sendActionBar(Component message) { sendActionBar(message.toLegacy()); }

    void setPlayerListHeaderFooter(String header, String footer);

    default void sendPlayerListHeaderAndFooter(Component header, Component footer) {
        setPlayerListHeaderFooter(header.toLegacy(), footer.toLegacy());
    }

    void playSound(Location location, String sound, float volume, float pitch);

    InetSocketAddress getAddress();

    int getPing();

    String getLocale();

    void updateInventory();

    Location getCompassTarget();

    Location getBedSpawnLocation();

    default Location getRespawnLocation() { return getBedSpawnLocation(); }

    boolean getAllowFlight();

    void setAllowFlight(boolean flight);

    void setFlying(boolean value);

    default boolean isConnected() { return isOnline(); }

    String getClientBrandName();

    // ---- Inventare ----
    org.bukkit.inventory.InventoryView openInventory(org.bukkit.inventory.Inventory inventory);

    org.bukkit.inventory.InventoryView getOpenInventory();

    // ---- Scoreboard ----
    org.bukkit.scoreboard.Scoreboard getScoreboard();

    void setScoreboard(org.bukkit.scoreboard.Scoreboard scoreboard);

    // ---- Sichtbarkeit ----
    void hidePlayer(org.bukkit.plugin.Plugin plugin, Player player);

    void showPlayer(org.bukkit.plugin.Plugin plugin, Player player);

    @Deprecated
    default void hidePlayer(Player player) { hidePlayer(null, player); }

    @Deprecated
    default void showPlayer(Player player) { showPlayer(null, player); }

    boolean canSee(Player player);

    // ---- Geschwindigkeit (Bukkit-Werte: Fliegen 0.1, Laufen 0.2) ----
    float getFlySpeed();

    void setFlySpeed(float value);

    float getWalkSpeed();

    void setWalkSpeed(float value);

    // ---- Partikel (nur für diesen Spieler) ----
    <T> void spawnParticle(org.bukkit.Particle particle, double x, double y, double z, int count, double offsetX, double offsetY,
                           double offsetZ, double extra, T data);

    default void spawnParticle(org.bukkit.Particle particle, org.bukkit.Location l, int count) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, 0, 0, 0, 0, null);
    }

    default void spawnParticle(org.bukkit.Particle particle, double x, double y, double z, int count) {
        spawnParticle(particle, x, y, z, count, 0, 0, 0, 0, null);
    }

    default <T> void spawnParticle(org.bukkit.Particle particle, org.bukkit.Location l, int count, T data) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, 0, 0, 0, 0, data);
    }

    default void spawnParticle(org.bukkit.Particle particle, org.bukkit.Location l, int count, double ox, double oy, double oz) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, 0, null);
    }

    default void spawnParticle(org.bukkit.Particle particle, org.bukkit.Location l, int count, double ox, double oy, double oz, double extra) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, extra, null);
    }

    default <T> void spawnParticle(org.bukkit.Particle particle, org.bukkit.Location l, int count, double ox, double oy, double oz, T data) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, 0, data);
    }

    default <T> void spawnParticle(org.bukkit.Particle particle, org.bukkit.Location l, int count, double ox, double oy, double oz,
                                   double extra, T data) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, extra, data);
    }

    // ---- Adventure-Bossbars ----
    void showBossBar(net.kyori.adventure.bossbar.BossBar bar);

    void hideBossBar(net.kyori.adventure.bossbar.BossBar bar);
}
