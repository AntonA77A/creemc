package org.bukkit.scoreboard;

/** Kriterium eines Ziels (in CreeMC zählen alle wie DUMMY: Werte setzt das Plugin). */
public final class Criteria {
    public static final Criteria DUMMY = new Criteria("dummy");
    public static final Criteria HEALTH = new Criteria("health");
    public static final Criteria DEATH_COUNT = new Criteria("deathCount");
    public static final Criteria PLAYER_KILL_COUNT = new Criteria("playerKillCount");
    public static final Criteria TOTAL_KILL_COUNT = new Criteria("totalKillCount");
    public static final Criteria LEVEL = new Criteria("level");
    public static final Criteria FOOD = new Criteria("food");
    public static final Criteria XP = new Criteria("xp");

    private final String name;

    private Criteria(String name) { this.name = name; }

    public static Criteria create(String name) { return new Criteria(name); }

    public String getName() { return name; }

    public boolean isReadOnly() { return !name.equals("dummy"); }

    public RenderType getDefaultRenderType() { return name.equals("health") ? RenderType.HEARTS : RenderType.INTEGER; }
}