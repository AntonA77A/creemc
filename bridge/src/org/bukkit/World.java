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

    boolean createExplosion(double x, double y, double z, float power, boolean setFire, boolean breakBlocks);

    default boolean createExplosion(Location l, float power, boolean setFire, boolean breakBlocks) {
        return createExplosion(l.getX(), l.getY(), l.getZ(), power, setFire, breakBlocks);
    }

    // ---- Entities ----
    List<org.bukkit.entity.Entity> getEntities();

    default List<org.bukkit.entity.LivingEntity> getLivingEntities() {
        List<org.bukkit.entity.LivingEntity> out = new java.util.ArrayList<>();
        for (org.bukkit.entity.Entity e : getEntities()) if (e instanceof org.bukkit.entity.LivingEntity l) out.add(l);
        return out;
    }

    default java.util.Collection<org.bukkit.entity.Entity> getNearbyEntities(Location l, double dx, double dy, double dz) {
        List<org.bukkit.entity.Entity> out = new java.util.ArrayList<>();
        for (org.bukkit.entity.Entity e : getEntities()) {
            Location o = e.getLocation();
            if (Math.abs(o.getX() - l.getX()) <= dx && Math.abs(o.getY() - l.getY()) <= dy && Math.abs(o.getZ() - l.getZ()) <= dz) out.add(e);
        }
        for (Player p : getPlayers()) {
            Location o = p.getLocation();
            if (Math.abs(o.getX() - l.getX()) <= dx && Math.abs(o.getY() - l.getY()) <= dy && Math.abs(o.getZ() - l.getZ()) <= dz) out.add(p);
        }
        return out;
    }

    /** Mob erzeugen (CreeMC kennt pig, cow, sheep, chicken, zombie, spider, creeper; TNT = angezündetes TNT). */
    org.bukkit.entity.Entity spawnEntity(Location location, org.bukkit.entity.EntityType type);

    void strikeLightningEffect(Location location);

    // ---- Spielregeln ----
    <T> T getGameRuleValue(GameRule<T> rule);

    <T> boolean setGameRule(GameRule<T> rule, T value);

    default <T> T getGameRuleDefault(GameRule<T> rule) { return getGameRuleValue(rule); }

    String getGameRuleValue(String rule);

    boolean setGameRuleValue(String rule, String value);

    default boolean isGameRule(String rule) { return getGameRuleValue(rule) != null; }

    default String[] getGameRules() {
        GameRule<?>[] all = GameRule.values();
        String[] out = new String[all.length];
        for (int i = 0; i < all.length; i++) out[i] = all[i].getName();
        return out;
    }

    // ---- Partikel (für alle in der Nähe) ----
    <T> void spawnParticle(Particle particle, double x, double y, double z, int count, double offsetX, double offsetY, double offsetZ,
                           double extra, T data, boolean force);

    default void spawnParticle(Particle particle, Location l, int count) { spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, 0, 0, 0, 0, null, false); }

    default void spawnParticle(Particle particle, double x, double y, double z, int count) { spawnParticle(particle, x, y, z, count, 0, 0, 0, 0, null, false); }

    default <T> void spawnParticle(Particle particle, Location l, int count, T data) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, 0, 0, 0, 0, data, false);
    }

    default void spawnParticle(Particle particle, Location l, int count, double ox, double oy, double oz) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, 0, null, false);
    }

    default void spawnParticle(Particle particle, Location l, int count, double ox, double oy, double oz, double extra) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, extra, null, false);
    }

    default <T> void spawnParticle(Particle particle, Location l, int count, double ox, double oy, double oz, T data) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, 0, data, false);
    }

    default <T> void spawnParticle(Particle particle, Location l, int count, double ox, double oy, double oz, double extra, T data) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, extra, data, false);
    }

    default <T> void spawnParticle(Particle particle, Location l, int count, double ox, double oy, double oz, double extra, T data,
                                   boolean force) {
        spawnParticle(particle, l.getX(), l.getY(), l.getZ(), count, ox, oy, oz, extra, data, force);
    }
}
