package org.bukkit.scoreboard;

import java.util.Set;
import net.kyori.adventure.text.Component;
import org.bukkit.OfflinePlayer;

public interface Scoreboard {
    Objective registerNewObjective(String name, String criteria);

    Objective registerNewObjective(String name, String criteria, String displayName);

    Objective registerNewObjective(String name, String criteria, String displayName, RenderType renderType);

    Objective registerNewObjective(String name, Criteria criteria, String displayName);

    Objective registerNewObjective(String name, Criteria criteria, String displayName, RenderType renderType);

    default Objective registerNewObjective(String name, Criteria criteria, Component displayName) {
        return registerNewObjective(name, criteria, displayName == null ? name : displayName.toLegacy());
    }

    default Objective registerNewObjective(String name, Criteria criteria, Component displayName, RenderType renderType) {
        return registerNewObjective(name, criteria, displayName == null ? name : displayName.toLegacy(), renderType);
    }

    Objective getObjective(String name);

    Objective getObjective(DisplaySlot slot);

    Set<Objective> getObjectives();

    Set<Objective> getObjectivesByCriteria(String criteria);

    void clearSlot(DisplaySlot slot);

    Team registerNewTeam(String name);

    Team getTeam(String name);

    Set<Team> getTeams();

    Team getEntryTeam(String entry);

    @Deprecated
    default Team getPlayerTeam(OfflinePlayer player) { return getEntryTeam(player.getName()); }

    Set<Score> getScores(String entry);

    void resetScores(String entry);

    @Deprecated
    default void resetScores(OfflinePlayer player) { resetScores(player.getName()); }

    Set<String> getEntries();
}