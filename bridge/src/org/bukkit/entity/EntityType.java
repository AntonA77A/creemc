package org.bukkit.entity;

public enum EntityType {
    PLAYER, ITEM, PIG, COW, SHEEP, CHICKEN, ZOMBIE, SPIDER, CREEPER, SKELETON, TNT, UNKNOWN;

    public static EntityType fromName(String name) {
        if (name == null) return UNKNOWN;
        try {
            return valueOf(name.replace("minecraft:", "").toUpperCase());
        } catch (IllegalArgumentException e) {
            return UNKNOWN;
        }
    }
}
