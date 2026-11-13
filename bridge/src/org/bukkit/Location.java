package org.bukkit;

import org.bukkit.block.Block;
import org.bukkit.util.Vector;

public class Location implements Cloneable {
    private World world;
    private double x, y, z;
    private float yaw, pitch;

    public Location(World world, double x, double y, double z) { this(world, x, y, z, 0, 0); }

    public Location(World world, double x, double y, double z, float yaw, float pitch) {
        this.world = world;
        this.x = x;
        this.y = y;
        this.z = z;
        this.yaw = yaw;
        this.pitch = pitch;
    }

    public World getWorld() { return world; }

    public void setWorld(World world) { this.world = world; }

    public boolean isWorldLoaded() { return world != null; }

    public Block getBlock() { return world.getBlockAt(this); }

    public double getX() { return x; }

    public double getY() { return y; }

    public double getZ() { return z; }

    public void setX(double x) { this.x = x; }

    public void setY(double y) { this.y = y; }

    public void setZ(double z) { this.z = z; }

    public float getYaw() { return yaw; }

    public float getPitch() { return pitch; }

    public void setYaw(float yaw) { this.yaw = yaw; }

    public void setPitch(float pitch) { this.pitch = pitch; }

    public int getBlockX() { return (int) Math.floor(x); }

    public int getBlockY() { return (int) Math.floor(y); }

    public int getBlockZ() { return (int) Math.floor(z); }

    public Location add(double dx, double dy, double dz) {
        x += dx;
        y += dy;
        z += dz;
        return this;
    }

    public Location add(Location o) { return add(o.x, o.y, o.z); }

    public Location add(Vector v) { return add(v.getX(), v.getY(), v.getZ()); }

    public Location subtract(double dx, double dy, double dz) { return add(-dx, -dy, -dz); }

    public Location subtract(Location o) { return add(-o.x, -o.y, -o.z); }

    public Location subtract(Vector v) { return add(-v.getX(), -v.getY(), -v.getZ()); }

    public Location multiply(double m) {
        x *= m;
        y *= m;
        z *= m;
        return this;
    }

    public Location zero() {
        x = y = z = 0;
        return this;
    }

    public double length() { return Math.sqrt(x * x + y * y + z * z); }

    public double distanceSquared(Location o) {
        double dx = x - o.x, dy = y - o.y, dz = z - o.z;
        return dx * dx + dy * dy + dz * dz;
    }

    public double distance(Location o) { return Math.sqrt(distanceSquared(o)); }

    public Vector toVector() { return new Vector(x, y, z); }

    public Vector getDirection() {
        double ry = Math.toRadians(yaw), rp = Math.toRadians(pitch);
        double xz = Math.cos(rp);
        return new Vector(-xz * Math.sin(ry), -Math.sin(rp), xz * Math.cos(ry));
    }

    public Location setDirection(Vector v) {
        double vx = v.getX(), vz = v.getZ();
        if (vx == 0 && vz == 0) {
            pitch = v.getY() > 0 ? -90 : 90;
            return this;
        }
        yaw = (float) Math.toDegrees((Math.atan2(-vx, vz) + Math.PI * 2) % (Math.PI * 2));
        pitch = (float) Math.toDegrees(Math.atan(-v.getY() / Math.sqrt(vx * vx + vz * vz)));
        return this;
    }

    @Override
    public Location clone() { return new Location(world, x, y, z, yaw, pitch); }

    @Override
    public boolean equals(Object o) {
        return o instanceof Location l && l.world == world && l.x == x && l.y == y && l.z == z && l.yaw == yaw && l.pitch == pitch;
    }

    @Override
    public int hashCode() { return java.util.Objects.hash(x, y, z, yaw, pitch); }

    @Override
    public String toString() {
        return "Location{world=" + (world == null ? null : world.getName()) + ",x=" + x + ",y=" + y + ",z=" + z
                + ",pitch=" + pitch + ",yaw=" + yaw + "}";
    }
}
