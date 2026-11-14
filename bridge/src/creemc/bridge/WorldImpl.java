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
    // ---- erweiterte API ----

    @Override
    public boolean createExplosion(double x, double y, double z, float power, boolean setFire, boolean breakBlocks) {
        bridge.call("explode", "x", x, "y", y, "z", z, "power", (double) power, "break", breakBlocks);
        return true;
    }

    @Override
    public List<org.bukkit.entity.Entity> getEntities() {
        List<org.bukkit.entity.Entity> out = new java.util.ArrayList<>();
        for (Object o : Json.list(bridge.request("entities"))) out.add(bridge.server.entity(Json.obj(o)));
        return out;
    }

    @Override
    public org.bukkit.entity.Entity spawnEntity(Location l, org.bukkit.entity.EntityType type) {
        if (type == org.bukkit.entity.EntityType.TNT) {
            bridge.call("tnt", "x", l.getX(), "y", l.getY(), "z", l.getZ(), "fuse", 80);
            return null;
        }
        int id = (int) Json.num(bridge.request("spawn_mob", "type", type.name().toLowerCase(), "x", l.getX(), "y", l.getY(), "z", l.getZ()), -1);
        if (id < 0) throw new IllegalArgumentException("CreeMC kann " + type + " (noch) nicht erzeugen");
        Object snap = bridge.request("entity", "id", id);
        return snap == null ? null : bridge.server.entity(Json.obj(snap));
    }

    @Override
    public void strikeLightningEffect(Location l) {
        playSound(l, "entity.lightning_bolt.thunder", 10000f, 0.8f);
        spawnParticle(org.bukkit.Particle.FLASH, l.getX(), l.getY() + 1, l.getZ(), 1, 0, 0, 0, 0, null, true);
    }

    @Override
    @SuppressWarnings("unchecked")
    public <T> T getGameRuleValue(org.bukkit.GameRule<T> rule) {
        String v = getGameRuleValue(rule.getName());
        if (v == null) return null;
        return rule.getType() == Boolean.class ? (T) Boolean.valueOf(v) : (T) Integer.valueOf(v);
    }

    @Override
    public <T> boolean setGameRule(org.bukkit.GameRule<T> rule, T value) { return setGameRuleValue(rule.getName(), String.valueOf(value)); }

    @Override
    public String getGameRuleValue(String rule) {
        Object v = bridge.request("gamerule", "rule", rule);
        return v == null ? null : String.valueOf(v);
    }

    @Override
    public boolean setGameRuleValue(String rule, String value) { return Boolean.TRUE.equals(bridge.request("set_gamerule", "rule", rule, "value", value)); }

    @Override
    public <T> void spawnParticle(org.bukkit.Particle particle, double x, double y, double z, int count, double ox, double oy, double oz,
                                  double extra, T data, boolean force) {
        bridge.call("particle", particleArgs(null, particle, x, y, z, count, ox, oy, oz, extra, data, force));
    }

    /** Auftrag "particle" für den Server (to = null: alle in der Nähe). */
    static Object[] particleArgs(String to, org.bukkit.Particle particle, double x, double y, double z, int count, double ox, double oy,
                                 double oz, double extra, Object data, boolean force) {
        List<Object> a = new java.util.ArrayList<>(List.of("name", particle.getKey().asString(), "x", x, "y", y, "z", z, "dx", ox, "dy", oy,
                "dz", oz, "speed", extra, "count", count, "force", force));
        if (to != null) a.addAll(List.of("to", to));
        if (data instanceof org.bukkit.Particle.DustOptions d) a.addAll(List.of("color", d.getColor().asRGB(), "size", (double) d.getSize()));
        else if (data instanceof org.bukkit.Color c) a.addAll(List.of("color", c.asRGB()));
        else if (data instanceof org.bukkit.Material m) a.addAll(List.of("block", m.getKey().asString()));
        else if (data instanceof String s) a.addAll(List.of("block", s));
        return a.toArray();
    }
}