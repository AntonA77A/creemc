package creemc.bridge;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.bukkit.Location;
import org.bukkit.World;
import org.bukkit.block.Block;
import org.bukkit.entity.Item;
import org.bukkit.entity.Player;
import org.bukkit.inventory.ItemStack;

public final class WorldImpl implements World {
    private final Bridge bridge;
    private final String name;
    private final UUID uid;
    private final double[] spawn = {0.5, 64, 0.5};
    private final int minY, maxY;
    volatile long time;

    WorldImpl(Bridge bridge, String name, List<Object> spawn, int minY, int maxY) {
        this.bridge = bridge;
        this.name = name;
        this.uid = UUID.nameUUIDFromBytes(("CreeMC:" + name).getBytes(StandardCharsets.UTF_8));
        for (int i = 0; i < Math.min(3, spawn.size()); i++) this.spawn[i] = Json.num(spawn.get(i), this.spawn[i]);
        this.minY = minY;
        this.maxY = maxY;
    }

    BlockImpl block(int x, int y, int z, String known) { return new BlockImpl(bridge, this, x, y, z, known); }

    @Override public String getName() { return name; }

    @Override public UUID getUID() { return uid; }

    @Override public Environment getEnvironment() { return Environment.NORMAL; }

    @Override public Block getBlockAt(int x, int y, int z) { return block(x, y, z, null); }

    @Override public Block getBlockAt(Location l) { return getBlockAt(l.getBlockX(), l.getBlockY(), l.getBlockZ()); }

    @Override public int getHighestBlockYAt(int x, int z) { return (int) Json.num(bridge.request("highest", "x", x, "z", z), minY); }

    @Override public Block getHighestBlockAt(int x, int z) { return getBlockAt(x, getHighestBlockYAt(x, z), z); }

    @Override public Location getSpawnLocation() { return new Location(this, spawn[0], spawn[1], spawn[2]); }

    @Override
    public boolean setSpawnLocation(int x, int y, int z) {
        spawn[0] = x + 0.5;
        spawn[1] = y;
        spawn[2] = z + 0.5;
        bridge.call("set_spawn", "x", x, "y", y, "z", z);
        return true;
    }

    @Override public long getTime() { return time % 24000; }

    @Override
    public void setTime(long t) {
        long full = time - time % 24000 + t;
        setFullTime(full < time ? full + 24000 : full);
    }

    @Override public long getFullTime() { return time; }

    @Override
    public void setFullTime(long t) {
        time = t;
        bridge.call("time", "value", t);
    }

    @Override
    public List<Player> getPlayers() { return new ArrayList<>(bridge.server.getOnlinePlayers()); }

    @Override
    public Item dropItem(Location l, ItemStack item) {
        bridge.call("drop", "x", l.getX(), "y", l.getY(), "z", l.getZ(), "item", item.toCreeItem(), "count", item.getAmount(), "scatter", false);
        return new ItemImpl(bridge, item, l);
    }

    @Override
    public Item dropItemNaturally(Location l, ItemStack item) {
        bridge.call("drop", "x", l.getX(), "y", l.getY(), "z", l.getZ(), "item", item.toCreeItem(), "count", item.getAmount(), "scatter", true);
        return new ItemImpl(bridge, item, l);
    }

    @Override public int getMinHeight() { return minY; }

    @Override public int getMaxHeight() { return maxY; }

    @Override public boolean isDayTime() { long t = getTime(); return t < 12300 || t > 23850; }

    @Override
    public void playSound(Location l, String sound, float volume, float pitch) {
        bridge.call("sound", "to", null, "name", sound.contains(":") ? sound : "minecraft:" + sound,
                "x", l.getX(), "y", l.getY(), "z", l.getZ(), "volume", volume, "pitch", pitch);
    }

    @Override public void createExplosion(Location l, float power) { createExplosion(l.getX(), l.getY(), l.getZ(), power); }

    @Override
    public boolean createExplosion(double x, double y, double z, float power) {
        bridge.call("explode", "x", x, "y", y, "z", z, "power", power);
        return true;
    }

    @Override public boolean equals(Object o) { return o instanceof World w && w.getUID().equals(uid); }

    @Override public int hashCode() { return uid.hashCode(); }

    @Override public String toString() { return "CraftWorld{name=" + name + "}"; }
}
