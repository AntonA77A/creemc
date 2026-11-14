package org.bukkit.entity;

public enum EntityType {
    PLAYER, ITEM, PIG, COW, SHEEP, CHICKEN, ZOMBIE, SPIDER, CREEPER, SKELETON, TNT, FALLING_BLOCK, EXPERIENCE_ORB, ARROW, UNKNOWN;

    public boolean isAlive() { return this != ITEM && this != TNT && this != FALLING_BLOCK && this != EXPERIENCE_ORB && this != ARROW && this != UNKNOWN; }

    public boolean isSpawnable() { return this != PLAYER && this != UNKNOWN; }

    public org.bukkit.NamespacedKey getKey() { return org.bukkit.NamespacedKey.minecraft(name().toLowerCase()); }

    public static EntityType fromName(String name) {
        if (name == null) return UNKNOWN;
        try {
            return valueOf(name.replace("minecraft:", "").toUpperCase());
        } catch (IllegalArgumentException e) {
            return UNKNOWN;
        }
    }
}
