package creemc.bridge;

import java.util.Map;
import java.util.UUID;
import org.bukkit.Bukkit;
import org.bukkit.Location;
import org.bukkit.Server;
import org.bukkit.World;
import org.bukkit.entity.Entity;
import org.bukkit.entity.EntityType;
import org.bukkit.entity.LivingEntity;
import org.bukkit.util.Vector;

/** Mob oder anderes Entity (Spiegel aus dem letzten Schnappschuss). */
class EntityImpl implements LivingEntity {
    protected final Bridge bridge;
    protected final int id;
    protected UUID uuid = UUID.randomUUID();
    protected String type = "unknown";
    protected double x, y, z, health = 20, maxHealth = 20;
    protected float yaw, pitch;
    protected boolean onGround, dead;
    protected int fireTicks;

    EntityImpl(Bridge bridge, int id) {
        this.bridge = bridge;
        this.id = id;
    }

    void update(Map<String, Object> s) {
        if (s.get("uuid") != null) uuid = UUID.fromString(Json.str(s.get("uuid"), ""));
        if (s.get("type") != null) type = Json.str(s.get("type"), type);
        x = Json.num(s.get("x"), x);
        y = Json.num(s.get("y"), y);
        z = Json.num(s.get("z"), z);
        yaw = (float) Json.num(s.get("yaw"), yaw);
        pitch = (float) Json.num(s.get("pitch"), pitch);
        health = Json.num(s.get("health"), health);
        maxHealth = Json.num(s.get("max_health"), maxHealth);
        onGround = Boolean.TRUE.equals(s.getOrDefault("ground", onGround));
        dead = Boolean.TRUE.equals(s.getOrDefault("dead", dead));
        fireTicks = (int) Json.num(s.get("fire"), fireTicks);
    }

    @Override public Location getLocation() { return new Location(bridge.server.world, x, y, z, yaw, pitch); }

    @Override
    public boolean teleport(Location l) {
        bridge.call("entity_tp", "id", id, "x", l.getX(), "y", l.getY(), "z", l.getZ());
        x = l.getX();
        y = l.getY();
        z = l.getZ();
        return true;
    }

    @Override public boolean teleport(Entity destination) { return teleport(destination.getLocation()); }

    @Override public World getWorld() { return bridge.server.world; }

    @Override public UUID getUniqueId() { return uuid; }

    @Override public int getEntityId() { return id; }

    @Override public EntityType getType() { return EntityType.fromName(type); }

    @Override public boolean isDead() { return dead; }

    @Override public boolean isValid() { return !dead; }

    @Override
    public void remove() {
        bridge.call("remove_entity", "id", id);
        dead = true;
    }

    @Override public boolean isOnGround() { return onGround; }

    @Override public Vector getVelocity() { return new Vector(); }

    @Override public void setVelocity(Vector v) { bridge.call("entity_velocity", "id", id, "x", v.getX(), "y", v.getY(), "z", v.getZ()); }

    @Override public int getFireTicks() { return fireTicks; }

    @Override
    public void setFireTicks(int ticks) {
        fireTicks = ticks;
        bridge.call("entity_fire", "id", id, "ticks", ticks);
    }

    @Override public double getHealth() { return health; }

    @Override
    public void setHealth(double h) {
        health = h;
        bridge.call("entity_health", "id", id, "value", h);
    }

    @Override public double getMaxHealth() { return maxHealth; }

    @Override public void damage(double amount) { bridge.call("entity_damage", "id", id, "amount", amount); }

    @Override
    public void damage(double amount, Entity source) {
        bridge.call("entity_damage", "id", id, "amount", amount, "source", source == null ? null : source.getUniqueId().toString());
    }

    @Override public Location getEyeLocation() { return getLocation().add(0, getEyeHeight(), 0); }

    @Override public double getEyeHeight() { return 1.5; }

    @Override public int getRemainingAir() { return 300; }

    @Override public void setRemainingAir(int ticks) { }

    @Override public int getMaximumAir() { return 300; }

    // CommandSender (Entities können in Bukkit Nachrichten "empfangen")
    @Override public void sendMessage(String message) { }

    @Override public void sendMessage(String... messages) { }

    @Override public Server getServer() { return Bukkit.getServer(); }

    @Override public String getName() { return type.substring(0, 1).toUpperCase() + type.substring(1); }

    @Override public boolean hasPermission(String name) { return false; }

    @Override public boolean isPermissionSet(String name) { return false; }

    @Override public boolean isOp() { return false; }

    @Override public void setOp(boolean value) { }

    @Override public boolean equals(Object o) { return o instanceof Entity e && e.getEntityId() == id; }

    @Override public int hashCode() { return id; }

    @Override public String toString() { return "CreeEntity{" + type + "#" + id + "}"; }
}
