package org.bukkit.block;

import org.bukkit.Location;
import org.bukkit.Material;
import org.bukkit.World;

public interface Block {
    Material getType();

    void setType(Material type);

    void setType(Material type, boolean applyPhysics);

    int getX();

    int getY();

    int getZ();

    World getWorld();

    Location getLocation();

    Block getRelative(int dx, int dy, int dz);

    Block getRelative(BlockFace face);

    Block getRelative(BlockFace face, int distance);

    boolean isEmpty();

    boolean isLiquid();

    boolean isSolid();

    boolean breakNaturally();

    /** CreeMC: kompletter Blockzustand, z.B. "minecraft:oak_stairs[facing=north,...]" */
    String getBlockDataString();
}
