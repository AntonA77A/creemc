package org.bukkit;

import java.util.LinkedHashMap;
import java.util.Map;

/** Spielregeln wie in Vanilla: world.setGameRule(GameRule.FALL_DAMAGE, false). */
public final class GameRule<T> {
    private static final Map<String, GameRule<?>> BY_NAME = new LinkedHashMap<>();

    /** Fortschritte im Chat ankündigen */
    public static final GameRule<Boolean> ANNOUNCE_ADVANCEMENTS = add("announceAdvancements", Boolean.class);
    /** Block-Explosionen droppen nur einen Teil */
    public static final GameRule<Boolean> BLOCK_EXPLOSION_DROP_DECAY = add("blockExplosionDropDecay", Boolean.class);
    /** Befehlsblöcke melden Ausgaben */
    public static final GameRule<Boolean> COMMAND_BLOCK_OUTPUT = add("commandBlockOutput", Boolean.class);
    /** Elytra-Geschwindigkeit nicht prüfen */
    public static final GameRule<Boolean> DISABLE_ELYTRA_MOVEMENT_CHECK = add("disableElytraMovementCheck", Boolean.class);
    /** Keine Überfälle */
    public static final GameRule<Boolean> DISABLE_RAIDS = add("disableRaids", Boolean.class);
    /** Tag/Nacht-Wechsel */
    public static final GameRule<Boolean> DO_DAYLIGHT_CYCLE = add("doDaylightCycle", Boolean.class);
    /** Entities (Loren, Rahmen ...) droppen Items */
    public static final GameRule<Boolean> DO_ENTITY_DROPS = add("doEntityDrops", Boolean.class);
    /** Feuer breitet sich aus / brennt ab */
    public static final GameRule<Boolean> DO_FIRE_TICK = add("doFireTick", Boolean.class);
    /** Sofort wiederbeleben ohne Todesbildschirm */
    public static final GameRule<Boolean> DO_IMMEDIATE_RESPAWN = add("doImmediateRespawn", Boolean.class);
    /** Phantome erscheinen */
    public static final GameRule<Boolean> DO_INSOMNIA = add("doInsomnia", Boolean.class);
    /** Nur freigeschaltete Rezepte */
    public static final GameRule<Boolean> DO_LIMITED_CRAFTING = add("doLimitedCrafting", Boolean.class);
    /** Mobs droppen Items und Erfahrung */
    public static final GameRule<Boolean> DO_MOB_LOOT = add("doMobLoot", Boolean.class);
    /** Mobs spawnen von selbst */
    public static final GameRule<Boolean> DO_MOB_SPAWNING = add("doMobSpawning", Boolean.class);
    /** Plünderer-Patrouillen */
    public static final GameRule<Boolean> DO_PATROL_SPAWNING = add("doPatrolSpawning", Boolean.class);
    /** Abgebaute Blöcke droppen Items */
    public static final GameRule<Boolean> DO_TILE_DROPS = add("doTileDrops", Boolean.class);
    /** Fahrende Händler */
    public static final GameRule<Boolean> DO_TRADER_SPAWNING = add("doTraderSpawning", Boolean.class);
    /** Ranken wachsen */
    public static final GameRule<Boolean> DO_VINES_SPREAD = add("doVinesSpread", Boolean.class);
    /** Wärter spawnen */
    public static final GameRule<Boolean> DO_WARDEN_SPAWNING = add("doWardenSpawning", Boolean.class);
    /** Wetterwechsel */
    public static final GameRule<Boolean> DO_WEATHER_CYCLE = add("doWeatherCycle", Boolean.class);
    /** Schaden durch Ertrinken */
    public static final GameRule<Boolean> DROWNING_DAMAGE = add("drowningDamage", Boolean.class);
    /** Enderperlen verschwinden beim Tod */
    public static final GameRule<Boolean> ENDER_PEARLS_VANISH_ON_DEATH = add("enderPearlsVanishOnDeath", Boolean.class);
    /** Fallschaden */
    public static final GameRule<Boolean> FALL_DAMAGE = add("fallDamage", Boolean.class);
    /** Schaden durch Feuer und Lava */
    public static final GameRule<Boolean> FIRE_DAMAGE = add("fireDamage", Boolean.class);
    /** Wütende Mobs vergessen tote Spieler */
    public static final GameRule<Boolean> FORGIVE_DEAD_PLAYERS = add("forgiveDeadPlayers", Boolean.class);
    /** Schaden durch Pulverschnee */
    public static final GameRule<Boolean> FREEZE_DAMAGE = add("freezeDamage", Boolean.class);
    /** Boss-Sounds überall hörbar */
    public static final GameRule<Boolean> GLOBAL_SOUND_EVENTS = add("globalSoundEvents", Boolean.class);
    /** Inventar und Erfahrung beim Tod behalten */
    public static final GameRule<Boolean> KEEP_INVENTORY = add("keepInventory", Boolean.class);
    /** Lava bildet unendliche Quellen */
    public static final GameRule<Boolean> LAVA_SOURCE_CONVERSION = add("lavaSourceConversion", Boolean.class);
    /** Admin-Befehle ins Log */
    public static final GameRule<Boolean> LOG_ADMIN_COMMANDS = add("logAdminCommands", Boolean.class);
    /** Mob-Explosionen droppen nur einen Teil */
    public static final GameRule<Boolean> MOB_EXPLOSION_DROP_DECAY = add("mobExplosionDropDecay", Boolean.class);
    /** Mobs zerstören Blöcke (Creeper) */
    public static final GameRule<Boolean> MOB_GRIEFING = add("mobGriefing", Boolean.class);
    /** Leben regeneriert sich mit vollem Hunger */
    public static final GameRule<Boolean> NATURAL_REGENERATION = add("naturalRegeneration", Boolean.class);
    /** Geschosse zerstören Blöcke */
    public static final GameRule<Boolean> PROJECTILES_CAN_BREAK_BLOCKS = add("projectilesCanBreakBlocks", Boolean.class);
    /** Spieler können sich gegenseitig verletzen */
    public static final GameRule<Boolean> PVP = add("pvp", Boolean.class);
    /** F3 zeigt weniger Infos */
    public static final GameRule<Boolean> REDUCED_DEBUG_INFO = add("reducedDebugInfo", Boolean.class);
    /** Befehle melden ihr Ergebnis */
    public static final GameRule<Boolean> SEND_COMMAND_FEEDBACK = add("sendCommandFeedback", Boolean.class);
    /** Todesnachrichten im Chat */
    public static final GameRule<Boolean> SHOW_DEATH_MESSAGES = add("showDeathMessages", Boolean.class);
    /** Monster spawnen von selbst */
    public static final GameRule<Boolean> SPAWN_MONSTERS = add("spawnMonsters", Boolean.class);
    /** Zuschauer laden neue Chunks */
    public static final GameRule<Boolean> SPECTATORS_GENERATE_CHUNKS = add("spectatorsGenerateChunks", Boolean.class);
    /** TNT kann explodieren */
    public static final GameRule<Boolean> TNT_EXPLODES = add("tntExplodes", Boolean.class);
    /** TNT-Explosionen droppen nur einen Teil */
    public static final GameRule<Boolean> TNT_EXPLOSION_DROP_DECAY = add("tntExplosionDropDecay", Boolean.class);
    /** Wütende Mobs greifen alle an */
    public static final GameRule<Boolean> UNIVERSAL_ANGER = add("universalAnger", Boolean.class);
    /** Wasser bildet unendliche Quellen */
    public static final GameRule<Boolean> WATER_SOURCE_CONVERSION = add("waterSourceConversion", Boolean.class);
    /** Maximale Länge von Befehlsketten */
    public static final GameRule<Integer> MAX_COMMAND_CHAIN_LENGTH = add("maxCommandChainLength", Integer.class);
    /** Ab so vielen Entities auf einem Fleck gibt es Schaden */
    public static final GameRule<Integer> MAX_ENTITY_CRAMMING = add("maxEntityCramming", Integer.class);
    /** Prozent der Spieler, die zum Überspringen der Nacht schlafen müssen */
    public static final GameRule<Integer> PLAYERS_SLEEPING_PERCENTAGE = add("playersSleepingPercentage", Integer.class);
    /** Wachstums-Geschwindigkeit (Pflanzen) */
    public static final GameRule<Integer> RANDOM_TICK_SPEED = add("randomTickSpeed", Integer.class);
    /** Schneehöhe bei Schneefall */
    public static final GameRule<Integer> SNOW_ACCUMULATION_HEIGHT = add("snowAccumulationHeight", Integer.class);
    /** Radius der Spawn-Chunks */
    public static final GameRule<Integer> SPAWN_CHUNK_RADIUS = add("spawnChunkRadius", Integer.class);
    /** Radius um den Spawnpunkt */
    public static final GameRule<Integer> SPAWN_RADIUS = add("spawnRadius", Integer.class);

    private final String name;
    private final Class<T> type;

    private GameRule(String name, Class<T> type) {
        this.name = name;
        this.type = type;
    }

    private static <T> GameRule<T> add(String name, Class<T> type) {
        GameRule<T> r = new GameRule<>(name, type);
        BY_NAME.put(name, r);
        return r;
    }

    public String getName() { return name; }
    public Class<T> getType() { return type; }
    public NamespacedKey getKey() { return NamespacedKey.minecraft(name); }
    public static GameRule<?> getByName(String name) { return BY_NAME.get(name); }
    public static GameRule<?>[] values() { return BY_NAME.values().toArray(new GameRule<?>[0]); }
    @Override public String toString() { return "GameRule{" + name + "}"; }
}
