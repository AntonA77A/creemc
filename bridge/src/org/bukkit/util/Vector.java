package org.bukkit.util;

import org.bukkit.Location;
import org.bukkit.World;

public class Vector implements Cloneable {
    protected double x, y, z;

    public Vector() { }

    public Vector(int x, int y, int z) { this((double) x, y, z); }

    public Vector(float x, float y, float z) { this((double) x, y, z); }

    public Vector(double x, double y, double z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public double getX() { return x; }

    public double getY() { return y; }

    public double getZ() { return z; }

    public Vector setX(double v) { x = v; return this; }

    public Vector setY(double v) { y = v; return this; }

    public Vector setZ(double v) { z = v; return this; }

    public Vector add(Vector o) { x += o.x; y += o.y; z += o.z; return this; }

    public Vector subtract(Vector o) { x -= o.x; y -= o.y; z -= o.z; return this; }

    public Vector multiply(double m) { x *= m; y *= m; z *= m; return this; }

    public Vector multiply(int m) { return multiply((double) m); }

    public Vector multiply(Vector o) { x *= o.x; y *= o.y; z *= o.z; return this; }

    public double length() { return Math.sqrt(lengthSquared()); }

    public double lengthSquared() { return x * x + y * y + z * z; }

    public Vector normalize() { double l = length(); return l == 0 ? this : multiply(1 / l); }

    public double distance(Vector o) { return Math.sqrt(distanceSquared(o)); }

    public double distanceSquared(Vector o) { double dx = x - o.x, dy = y - o.y, dz = z - o.z; return dx * dx + dy * dy + dz * dz; }

    public double dot(Vector o) { return x * o.x + y * o.y + z * o.z; }

    public Vector crossProduct(Vector o) {
        double nx = y * o.z - o.y * z, ny = z * o.x - o.z * x, nz = x * o.y - o.x * y;
        x = nx; y = ny; z = nz;
        return this;
    }

    public Vector zero() { x = y = z = 0; return this; }

    public int getBlockX() { return (int) Math.floor(x); }

    public int getBlockY() { return (int) Math.floor(y); }

    public int getBlockZ() { return (int) Math.floor(z); }

    public Location toLocation(World world) { return new Location(world, x, y, z); }

    public Location toLocation(World world, float yaw, float pitch) { return new Location(world, x, y, z, yaw, pitch); }

    @Override
    public Vector clone() { return new Vector(x, y, z); }

    @Override
    public boolean equals(Object o) { return o instanceof Vector v && v.x == x && v.y == y && v.z == z; }

    @Override
    public int hashCode() { return java.util.Objects.hash(x, y, z); }

    @Override
    public String toString() { return x + "," + y + "," + z; }
}
