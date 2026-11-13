package creemc.bridge;

import org.bukkit.Location;
import org.bukkit.Material;
import org.bukkit.World;
import org.bukkit.block.Block;
import org.bukkit.block.BlockFace;

final class BlockImpl implements Block {
    private final Bridge bridge;
    private final WorldImpl world;
    private final int x, y, z;
    private String state;   // "minecraft:oak_stairs[facing=north]" oder null (noch nicht gefragt)

    BlockImpl(Bridge bridge, WorldImpl world, int x, int y, int z, String known) {
        this.bridge = bridge;
        this.world = world;
        this.x = x;
        this.y = y;
        this.z = z;
        this.state = known;
    }

    @Override
    public String getBlockDataString() {
        if (state == null) state = Json.str(bridge.request("block", "x", x, "y", y, "z", z), "minecraft:air");
        return state;
    }

    @Override
    public Material getType() {
        String s = getBlockDataString();
        int i = s.indexOf('[');
        return Material.fromKey(i < 0 ? s : s.substring(0, i));
    }

    @Override public void setType(Material type) { setType(type, true); }

    @Override
    public void setType(Material type, boolean applyPhysics) {
        state = type.getKey().asString();
        bridge.call("set_block", "x", x, "y", y, "z", z, "block", state, "physics", applyPhysics);
    }

    @Override public int getX() { return x; }

    @Override public int getY() { return y; }

    @Override public int getZ() { return z; }

    @Override public World getWorld() { return world; }

    @Override public Location getLocation() { return new Location(world, x, y, z); }

    @Override public Block getRelative(int dx, int dy, int dz) { return world.getBlockAt(x + dx, y + dy, z + dz); }

    @Override public Block getRelative(BlockFace face) { return getRelative(face, 1); }

    @Override
    public Block getRelative(BlockFace face, int d) { return getRelative(face.getModX() * d, face.getModY() * d, face.getModZ() * d); }

    @Override public boolean isEmpty() { return getType().isAir(); }

    @Override
    public boolean isLiquid() {
        Material m = getType();
        return m == Material.WATER || m == Material.LAVA;
    }

    @Override public boolean isSolid() { return getType().isSolid(); }

    @Override
    public boolean breakNaturally() {
        if (isEmpty()) return false;
        bridge.call("break_block", "x", x, "y", y, "z", z);
        state = "minecraft:air";
        return true;
    }

    @Override
    public boolean equals(Object o) { return o instanceof Block b && b.getX() == x && b.getY() == y && b.getZ() == z; }

    @Override public int hashCode() { return (y * 31 + x) * 31 + z; }

    @Override public String toString() { return "CraftBlock{x=" + x + ",y=" + y + ",z=" + z + "}"; }
}
