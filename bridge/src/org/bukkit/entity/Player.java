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
}
