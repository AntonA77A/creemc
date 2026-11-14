package org.bukkit.scoreboard;

import java.util.Set;
import net.kyori.adventure.text.Component;
import org.bukkit.ChatColor;
import org.bukkit.OfflinePlayer;

public interface Team {
    enum Option { NAME_TAG_VISIBILITY, DEATH_MESSAGE_VISIBILITY, COLLISION_RULE }

    enum OptionStatus { ALWAYS, NEVER, FOR_OTHER_TEAMS, FOR_OWN_TEAM }

    String getName();

    String getDisplayName();

    void setDisplayName(String name);

    String getPrefix();

    void setPrefix(String prefix);

    String getSuffix();

    void setSuffix(String suffix);

    default Component prefix() { return Component.text(getPrefix()); }

    default void prefix(Component c) { setPrefix(c == null ? "" : c.toLegacy()); }

    default Component suffix() { return Component.text(getSuffix()); }

    default void suffix(Component c) { setSuffix(c == null ? "" : c.toLegacy()); }

    default Component displayName() { return Component.text(getDisplayName()); }

    default void displayName(Component c) { setDisplayName(c == null ? getName() : c.toLegacy()); }

    ChatColor getColor();

    void setColor(ChatColor color);

    boolean allowFriendlyFire();

    void setAllowFriendlyFire(boolean enabled);

    boolean canSeeFriendlyInvisibles();

    void setCanSeeFriendlyInvisibles(boolean enabled);

    OptionStatus getOption(Option option);

    void setOption(Option option, OptionStatus status);

    Set<String> getEntries();

    int getSize();

    Scoreboard getScoreboard();

    void addEntry(String entry);

    default void addEntries(String... entries) { for (String e : entries) addEntry(e); }

    boolean removeEntry(String entry);

    boolean hasEntry(String entry);

    @Deprecated
    default void addPlayer(OfflinePlayer p) { addEntry(p.getName()); }

    @Deprecated
    default boolean removePlayer(OfflinePlayer p) { return removeEntry(p.getName()); }

    @Deprecated
    default boolean hasPlayer(OfflinePlayer p) { return hasEntry(p.getName()); }

    void unregister();
}