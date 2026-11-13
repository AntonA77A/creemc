package org.bukkit.block;

import org.bukkit.util.Vector;

public enum BlockFace {
    NORTH(0, 0, -1), EAST(1, 0, 0), SOUTH(0, 0, 1), WEST(-1, 0, 0), UP(0, 1, 0), DOWN(0, -1, 0),
    NORTH_EAST(1, 0, -1), NORTH_WEST(-1, 0, -1), SOUTH_EAST(1, 0, 1), SOUTH_WEST(-1, 0, 1), SELF(0, 0, 0);

    private final int dx, dy, dz;

    BlockFace(int dx, int dy, int dz) {
        this.dx = dx;
        this.dy = dy;
        this.dz = dz;
    }

    public int getModX() { return dx; }

    public int getModY() { return dy; }

    public int getModZ() { return dz; }

    public Vector getDirection() { return new Vector(dx, dy, dz).normalize(); }

    public BlockFace getOppositeFace() {
        return switch (this) {
            case NORTH -> SOUTH; case SOUTH -> NORTH; case EAST -> WEST; case WEST -> EAST;
            case UP -> DOWN; case DOWN -> UP; case NORTH_EAST -> SOUTH_WEST; case SOUTH_WEST -> NORTH_EAST;
            case NORTH_WEST -> SOUTH_EAST; case SOUTH_EAST -> NORTH_WEST; default -> SELF;
        };
    }

    public static BlockFace fromProtocol(int face) {
        return switch (face) { case 0 -> DOWN; case 1 -> UP; case 2 -> NORTH; case 3 -> SOUTH; case 4 -> WEST; case 5 -> EAST; default -> SELF; };
    }
}
