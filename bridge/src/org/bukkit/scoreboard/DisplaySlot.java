package org.bukkit.scoreboard;

public enum DisplaySlot {
    PLAYER_LIST(0), SIDEBAR(1), BELOW_NAME(2);

    private final int id;

    DisplaySlot(int id) { this.id = id; }

    /** Vanilla-Nummer der Anzeige. */
    public int getId() { return id; }
}