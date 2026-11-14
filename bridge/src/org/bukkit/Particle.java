package org.bukkit;

/** Partikel (Namen wie in Vanilla ab 1.20.5; welche ein Client kennt, hängt von seiner Version ab). */
public enum Particle {
    ANGRY_VILLAGER,
    ASH,
    BLOCK,
    BLOCK_MARKER,
    BUBBLE,
    BUBBLE_COLUMN_UP,
    BUBBLE_POP,
    CAMPFIRE_COSY_SMOKE,
    CAMPFIRE_SIGNAL_SMOKE,
    CHERRY_LEAVES,
    CLOUD,
    COMPOSTER,
    CRIMSON_SPORE,
    CRIT,
    CURRENT_DOWN,
    DAMAGE_INDICATOR,
    DOLPHIN,
    DRAGON_BREATH,
    DRIPPING_DRIPSTONE_LAVA,
    DRIPPING_DRIPSTONE_WATER,
    DRIPPING_HONEY,
    DRIPPING_LAVA,
    DRIPPING_OBSIDIAN_TEAR,
    DRIPPING_WATER,
    DUST,
    DUST_COLOR_TRANSITION,
    DUST_PILLAR,
    DUST_PLUME,
    EFFECT,
    EGG_CRACK,
    ELDER_GUARDIAN,
    ELECTRIC_SPARK,
    ENCHANT,
    ENCHANTED_HIT,
    END_ROD,
    ENTITY_EFFECT,
    EXPLOSION,
    EXPLOSION_EMITTER,
    FALLING_DRIPSTONE_LAVA,
    FALLING_DRIPSTONE_WATER,
    FALLING_DUST,
    FALLING_HONEY,
    FALLING_LAVA,
    FALLING_NECTAR,
    FALLING_OBSIDIAN_TEAR,
    FALLING_SPORE_BLOSSOM,
    FALLING_WATER,
    FIREWORK,
    FISHING,
    FLAME,
    FLASH,
    GLOW,
    GLOW_SQUID_INK,
    GUST,
    GUST_EMITTER_LARGE,
    GUST_EMITTER_SMALL,
    HAPPY_VILLAGER,
    HEART,
    INFESTED,
    INSTANT_EFFECT,
    ITEM,
    ITEM_COBWEB,
    ITEM_SLIME,
    ITEM_SNOWBALL,
    LANDING_HONEY,
    LANDING_LAVA,
    LANDING_OBSIDIAN_TEAR,
    LARGE_SMOKE,
    LAVA,
    MYCELIUM,
    NAUTILUS,
    NOTE,
    OMINOUS_SPAWNING,
    POOF,
    PORTAL,
    RAID_OMEN,
    RAIN,
    REVERSE_PORTAL,
    SCRAPE,
    SCULK_CHARGE,
    SCULK_CHARGE_POP,
    SCULK_SOUL,
    SHRIEK,
    SMALL_FLAME,
    SMALL_GUST,
    SMOKE,
    SNEEZE,
    SNOWFLAKE,
    SONIC_BOOM,
    SOUL,
    SOUL_FIRE_FLAME,
    SPIT,
    SPLASH,
    SPORE_BLOSSOM_AIR,
    SQUID_INK,
    SWEEP_ATTACK,
    TOTEM_OF_UNDYING,
    TRIAL_OMEN,
    TRIAL_SPAWNER_DETECTION,
    TRIAL_SPAWNER_DETECTION_OMINOUS,
    UNDERWATER,
    VAULT_CONNECTION,
    VIBRATION,
    WARPED_SPORE,
    WAX_OFF,
    WAX_ON,
    WHITE_ASH,
    WHITE_SMOKE,
    WITCH,
    BLOCK_CRUMBLE,
    COPPER_FIRE_FLAME,
    FIREFLY,
    GEYSER,
    GEYSER_BASE,
    GEYSER_PLUME,
    GEYSER_POOF,
    NOXIOUS_GAS,
    NOXIOUS_GAS_CLOUD,
    ORANGE_POPLAR_LEAVES,
    PALE_OAK_LEAVES,
    PAUSE_MOB_GROWTH,
    RED_POPLAR_LEAVES,
    RESET_MOB_GROWTH,
    SULFUR_BUBBLES,
    SULFUR_CUBE_GOO,
    TINTED_LEAVES,
    TRAIL,
    YELLOW_POPLAR_LEAVES;

    /** Farbiger Staub: world.spawnParticle(Particle.DUST, ort, 10, new Particle.DustOptions(Color.RED, 1.5f)). */
    public static final class DustOptions {
        private final Color color;
        private final float size;

        public DustOptions(Color color, float size) {
            this.color = color;
            this.size = size;
        }

        public Color getColor() { return color; }

        public float getSize() { return size; }
    }

    public NamespacedKey getKey() { return NamespacedKey.minecraft(name().toLowerCase()); }

    public NamespacedKey key() { return getKey(); }

    /** Welche Zusatzdaten das Partikel braucht (DustOptions, Material/Block-Text) oder Void. */
    public Class<?> getDataType() {
        return switch (this) {
            case DUST -> DustOptions.class;
            case BLOCK, FALLING_DUST, BLOCK_MARKER, DUST_PILLAR -> Material.class;
            default -> Void.class;
        };
    }
}
