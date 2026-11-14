package creemc.api;

import java.util.ArrayList;
import java.util.List;
import org.bukkit.Bukkit;
import org.bukkit.entity.Player;
import org.bukkit.scoreboard.Criteria;
import org.bukkit.scoreboard.DisplaySlot;
import org.bukkit.scoreboard.Objective;
import org.bukkit.scoreboard.Scoreboard;
import org.bukkit.scoreboard.Team;

/**
 * Seitenleiste rechts (eigenes Scoreboard pro Spieler), Zeilen von oben nach unten.
 *
 * <pre>
 * Sidebar sb = CreeMC.sidebar(player, "&amp;6&amp;lMein Server");
 * sb.lines("&amp;7Spieler: &amp;f" + online, "", "&amp;eviel Spaß!");
 * </pre>
 */
public final class Sidebar {
    private static final String[] KEYS = {"§0", "§1", "§2", "§3", "§4", "§5", "§6", "§7", "§8", "§9", "§a", "§b", "§c", "§d", "§e"};
    private final Player player;
    private final Scoreboard board;
    private final Objective objective;
    private final List<String> lines = new ArrayList<>();

    Sidebar(Player player, String title) {
        this.player = player;
        this.board = Bukkit.getScoreboardManager().getNewScoreboard();
        this.objective = board.registerNewObjective("creemc_sb", Criteria.DUMMY, CreeMC.color(title));
        objective.setDisplaySlot(DisplaySlot.SIDEBAR);
        player.setScoreboard(board);
    }

    public Sidebar title(String title) {
        objective.setDisplayName(CreeMC.color(title));
        return this;
    }

    /** Alle Zeilen setzen (höchstens 15). */
    public Sidebar lines(String... text) { return lines(List.of(text)); }

    public Sidebar lines(List<String> text) {
        int n = Math.min(15, text.size());
        for (int i = 0; i < n; i++) line(i, text.get(i));
        for (int i = n; i < lines.size(); i++) remove(i);
        while (lines.size() > n) lines.remove(lines.size() - 1);
        return this;
    }

    /** Eine Zeile (0 = oben) setzen. Text über Team-Präfix, damit gleiche Texte und Farben gehen. */
    public Sidebar line(int index, String text) {
        if (index < 0 || index >= 15) throw new IllegalArgumentException("Zeile 0 bis 14");
        while (lines.size() <= index) lines.add(null);
        String key = KEYS[index];
        Team t = board.getTeam("line" + index);
        if (t == null) {
            t = board.registerNewTeam("line" + index);
            t.addEntry(key);
        }
        t.setPrefix(CreeMC.color(text));
        if (lines.get(index) == null) objective.getScore(key).setScore(15 - index);
        lines.set(index, text);
        return this;
    }

    private void remove(int index) {
        String key = KEYS[index];
        board.resetScores(key);
        Team t = board.getTeam("line" + index);
        if (t != null) t.unregister();
    }

    /** Seitenleiste entfernen (zurück zum Haupt-Scoreboard). */
    public void hide() { player.setScoreboard(Bukkit.getScoreboardManager().getMainScoreboard()); }
}
