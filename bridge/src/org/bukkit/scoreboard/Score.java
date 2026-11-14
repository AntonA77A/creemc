package org.bukkit.scoreboard;

import net.kyori.adventure.text.Component;

public interface Score {
    String getEntry();

    Objective getObjective();

    int getScore();

    void setScore(int score);

    boolean isScoreSet();

    void resetScore();

    Scoreboard getScoreboard();

    /** Paper: eigener Anzeigetext statt des Eintragsnamens (null = Name). */
    void customName(Component name);

    Component customName();
}