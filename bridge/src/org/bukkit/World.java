package org.bukkit;

import java.util.List;
import java.util.UUID;
import org.bukkit.block.Block;
import org.bukkit.entity.Item;
import org.bukkit.entity.Player;
import org.bukkit.inventory.ItemStack;

public interface World {
    enum Environment { NORMAL, NETHER, THE_END, CUSTOM }

    String getName();

    UUID getUID();

    Environment getEnvironment();

    Block getBlockAt(int x, int y, int z);

    Block getBlockAt(Location location);

    int getHighestBlockYAt(int x, int z);

    Block getHighestBlockAt(int x, int z);

    Location getSpawnLocation();

    boolean setSpawnLocation(int x, int y, int z);

    long getTime();

    void setTime(long time);

    long getFullTime();

    void setFullTime(long time);

    List<Player> getPlayers();

    Item dropItem(Location location, org.bukkit.inventory.ItemStack item);

    Item dropItemNaturally(Location location, ItemStack item);

    int getMinHeight();

    int getMaxHeight();

    boolean isDayTime();

    void playSound(Location location, String sound, float volume, float pitch);

    void createExplosion(Location location, float power);

    boolean createExplosion(double x, double y, double z, float power);
}
