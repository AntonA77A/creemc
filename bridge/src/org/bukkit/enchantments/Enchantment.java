package org.bukkit.enchantments;

import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.Map;
import org.bukkit.NamespacedKey;

/** Verzauberungen (alle Vanilla-Verzauberungen mit ihren Höchststufen). */
public final class Enchantment {
    private static final Map<String, Enchantment> BY_KEY = new LinkedHashMap<>();

    public static final Enchantment AQUA_AFFINITY = add("aqua_affinity", 1, false);
    public static final Enchantment BANE_OF_ARTHROPODS = add("bane_of_arthropods", 5, false);
    public static final Enchantment BINDING_CURSE = add("binding_curse", 1, true);
    public static final Enchantment BLAST_PROTECTION = add("blast_protection", 4, false);
    public static final Enchantment BREACH = add("breach", 4, false);
    public static final Enchantment CHANNELING = add("channeling", 1, false);
    public static final Enchantment DENSITY = add("density", 5, false);
    public static final Enchantment DEPTH_STRIDER = add("depth_strider", 3, false);
    public static final Enchantment EFFICIENCY = add("efficiency", 5, false);
    public static final Enchantment FEATHER_FALLING = add("feather_falling", 4, false);
    public static final Enchantment FIRE_ASPECT = add("fire_aspect", 2, false);
    public static final Enchantment FIRE_PROTECTION = add("fire_protection", 4, false);
    public static final Enchantment FLAME = add("flame", 1, false);
    public static final Enchantment FORTUNE = add("fortune", 3, false);
    public static final Enchantment FROST_WALKER = add("frost_walker", 2, false);
    public static final Enchantment IMPALING = add("impaling", 5, false);
    public static final Enchantment INFINITY = add("infinity", 1, false);
    public static final Enchantment KNOCKBACK = add("knockback", 2, false);
    public static final Enchantment LOOTING = add("looting", 3, false);
    public static final Enchantment LOYALTY = add("loyalty", 3, false);
    public static final Enchantment LUCK_OF_THE_SEA = add("luck_of_the_sea", 3, false);
    public static final Enchantment LUNGE = add("lunge", 3, false);
    public static final Enchantment LURE = add("lure", 3, false);
    public static final Enchantment MENDING = add("mending", 1, false);
    public static final Enchantment MULTISHOT = add("multishot", 1, false);
    public static final Enchantment PIERCING = add("piercing", 4, false);
    public static final Enchantment POWER = add("power", 5, false);
    public static final Enchantment PROJECTILE_PROTECTION = add("projectile_protection", 4, false);
    public static final Enchantment PROTECTION = add("protection", 4, false);
    public static final Enchantment PUNCH = add("punch", 2, false);
    public static final Enchantment QUICK_CHARGE = add("quick_charge", 3, false);
    public static final Enchantment RESPIRATION = add("respiration", 3, false);
    public static final Enchantment RIPTIDE = add("riptide", 3, false);
    public static final Enchantment SHARPNESS = add("sharpness", 5, false);
    public static final Enchantment SILK_TOUCH = add("silk_touch", 1, false);
    public static final Enchantment SMITE = add("smite", 5, false);
    public static final Enchantment SOUL_SPEED = add("soul_speed", 3, false);
    public static final Enchantment SWEEPING_EDGE = add("sweeping_edge", 3, false);
    public static final Enchantment SWIFT_SNEAK = add("swift_sneak", 3, false);
    public static final Enchantment THORNS = add("thorns", 3, false);
    public static final Enchantment UNBREAKING = add("unbreaking", 3, false);
    public static final Enchantment VANISHING_CURSE = add("vanishing_curse", 1, true);
    public static final Enchantment WIND_BURST = add("wind_burst", 3, false);

    @Deprecated public static final Enchantment DAMAGE_ALL = SHARPNESS;
    @Deprecated public static final Enchantment DAMAGE_UNDEAD = SMITE;
    @Deprecated public static final Enchantment DAMAGE_ARTHROPODS = BANE_OF_ARTHROPODS;
    @Deprecated public static final Enchantment DURABILITY = UNBREAKING;
    @Deprecated public static final Enchantment DIG_SPEED = EFFICIENCY;
    @Deprecated public static final Enchantment LOOT_BONUS_BLOCKS = FORTUNE;
    @Deprecated public static final Enchantment LOOT_BONUS_MOBS = LOOTING;
    @Deprecated public static final Enchantment PROTECTION_ENVIRONMENTAL = PROTECTION;
    @Deprecated public static final Enchantment PROTECTION_FIRE = FIRE_PROTECTION;
    @Deprecated public static final Enchantment PROTECTION_FALL = FEATHER_FALLING;
    @Deprecated public static final Enchantment PROTECTION_EXPLOSIONS = BLAST_PROTECTION;
    @Deprecated public static final Enchantment PROTECTION_PROJECTILE = PROJECTILE_PROTECTION;
    @Deprecated public static final Enchantment OXYGEN = RESPIRATION;
    @Deprecated public static final Enchantment WATER_WORKER = AQUA_AFFINITY;
    @Deprecated public static final Enchantment ARROW_DAMAGE = POWER;
    @Deprecated public static final Enchantment ARROW_KNOCKBACK = PUNCH;
    @Deprecated public static final Enchantment ARROW_FIRE = FLAME;
    @Deprecated public static final Enchantment ARROW_INFINITE = INFINITY;
    @Deprecated public static final Enchantment LUCK = LUCK_OF_THE_SEA;

    private final NamespacedKey key;
    private final int maxLevel;
    private final boolean curse;

    private Enchantment(String key, int maxLevel, boolean curse) {
        this.key = NamespacedKey.minecraft(key);
        this.maxLevel = maxLevel;
        this.curse = curse;
    }

    private static Enchantment add(String key, int max, boolean curse) {
        Enchantment e = new Enchantment(key, max, curse);
        BY_KEY.put("minecraft:" + key, e);
        return e;
    }

    public NamespacedKey getKey() { return key; }
    public NamespacedKey key() { return key; }
    public String getName() { return key.getKey().toUpperCase(); }
    public int getMaxLevel() { return maxLevel; }
    public int getStartLevel() { return 1; }
    public boolean isCursed() { return curse; }
    public boolean isTreasure() { return curse || key.getKey().equals("mending") || key.getKey().equals("frost_walker") || key.getKey().equals("soul_speed") || key.getKey().equals("swift_sneak"); }
    public static Enchantment getByKey(NamespacedKey key) { return key == null ? null : BY_KEY.get(key.asString()); }
    public static Enchantment getByName(String name) { return name == null ? null : BY_KEY.get("minecraft:" + name.toLowerCase()); }
    public static Enchantment[] values() { return BY_KEY.values().toArray(new Enchantment[0]); }
    public static Collection<Enchantment> all() { return BY_KEY.values(); }
    @Override public String toString() { return "Enchantment[" + key + "]"; }
}
