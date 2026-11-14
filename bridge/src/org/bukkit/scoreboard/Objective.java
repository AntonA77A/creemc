package org.bukkit.scoreboard;

import net.kyori.adventure.text.Component;
import org.bukkit.OfflinePlayer;

public interface Objective {
    String getName();

    String getDisplayName();

    void setDisplayName(String name);

    default Component displayName() { return Component.text(getDisplayName()); }

    default void displayName(Component name) { setDisplayName(name == null ? getName() : name.toLegacy()); }

    String getCriteria();

    Criteria getTrackedCriteria();

    boolean isModifiable();

    Scoreboard getScoreboard();

    void unregister();

    void setDisplaySlot(DisplaySlot slot);

    DisplaySlot getDisplaySlot();

    void setRenderType(RenderType type);

    RenderType getRenderType();

    Score getScore(String entry);

    @Deprecated
    default Score getScore(OfflinePlayer player) { return getScore(player.getName()); }
}