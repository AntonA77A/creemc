package creemc.bridge;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import net.kyori.adventure.text.Component;
import org.bukkit.ChatColor;
import org.bukkit.entity.Player;
import org.bukkit.scoreboard.Criteria;
import org.bukkit.scoreboard.DisplaySlot;
import org.bukkit.scoreboard.Objective;
import org.bukkit.scoreboard.RenderType;
import org.bukkit.scoreboard.Score;
import org.bukkit.scoreboard.Scoreboard;
import org.bukkit.scoreboard.ScoreboardManager;
import org.bukkit.scoreboard.Team;

/**
 * Scoreboards wie Bukkit: der Zustand lebt hier, Änderungen gehen als Pakete an alle Spieler, die das Board sehen
 * (Hauptscoreboard = alle ohne eigenes Board).
 */
public final class Scoreboards implements ScoreboardManager {
    private final Bridge bridge;
    private final Board main = new Board();
    private final Map<UUID, Board> viewing = new ConcurrentHashMap<>();

    Scoreboards(Bridge bridge) { this.bridge = bridge; }

    @Override public Scoreboard getMainScoreboard() { return main; }

    @Override public Scoreboard getNewScoreboard() { return new Board(); }

    Scoreboard of(Player p) { return viewing.getOrDefault(p.getUniqueId(), main); }

    void set(Player p, Scoreboard sb) {
        Board old = (Board) of(p), nw = (Board) sb;
        if (old == nw) return;
        old.hideFrom(p);
        if (nw == main) viewing.remove(p.getUniqueId());
        else viewing.put(p.getUniqueId(), nw);
        nw.showTo(p);
    }

    /** Neuer Spieler: Hauptscoreboard zeigen. */
    void joined(Player p) {
        viewing.remove(p.getUniqueId());
        main.showTo(p);
    }

    void quit(Player p) { viewing.remove(p.getUniqueId()); }

    private static String uuid(Player p) { return p.getUniqueId().toString(); }

    private static int color(ChatColor c) { return c == null || c.ordinal() > 15 ? 21 : c.ordinal(); }

    private static String rule(Team.OptionStatus s, boolean collision) {
        return switch (s) {
            case NEVER -> "never";
            case FOR_OTHER_TEAMS -> collision ? "pushOtherTeams" : "hideForOtherTeams";
            case FOR_OWN_TEAM -> collision ? "pushOwnTeam" : "hideForOwnTeam";
            default -> "always";
        };
    }

    final class Board implements Scoreboard {
        final Map<String, Obj> objectives = new LinkedHashMap<>();
        final Map<String, TeamImpl> teams = new LinkedHashMap<>();
        final Map<DisplaySlot, Obj> slots = new LinkedHashMap<>();

        List<Player> viewers() {
            List<Player> out = new ArrayList<>();
            for (Player p : bridge.server.getOnlinePlayers()) if (of(p) == this) out.add(p);
            return out;
        }

        void send(String op, Object... kv) {
            for (Player p : viewers()) sendTo(p, op, kv);
        }

        void sendTo(Player p, String op, Object... kv) {
            Object[] all = new Object[kv.length + 2];
            all[0] = "to";
            all[1] = uuid(p);
            System.arraycopy(kv, 0, all, 2, kv.length);
            bridge.call(op, all);
        }

        void showTo(Player p) {
            for (Obj o : objectives.values()) {
                sendTo(p, "sb_objective", "name", o.name, "mode", 0, "title", o.display, "hearts", o.render == RenderType.HEARTS);
                for (ScoreImpl s : o.scores.values())
                    sendTo(p, "sb_score", "entry", s.entry, "objective", o.name, "value", s.value, "display", s.custom);
            }
            slots.forEach((slot, o) -> sendTo(p, "sb_display", "slot", slot.getId(), "name", o.name));
            for (TeamImpl t : teams.values()) t.sendCreate(p);
        }

        void hideFrom(Player p) {
            for (Obj o : objectives.values()) sendTo(p, "sb_objective", "name", o.name, "mode", 1);
            for (TeamImpl t : teams.values()) sendTo(p, "sb_team", "name", t.name, "mode", 1);
        }

        @Override public Objective registerNewObjective(String name, String criteria) { return registerNewObjective(name, criteria, name); }

        @Override
        public Objective registerNewObjective(String name, String criteria, String displayName) {
            return registerNewObjective(name, Criteria.create(criteria), displayName, RenderType.INTEGER);
        }

        @Override
        public Objective registerNewObjective(String name, String criteria, String displayName, RenderType renderType) {
            return registerNewObjective(name, Criteria.create(criteria), displayName, renderType);
        }

        @Override
        public Objective registerNewObjective(String name, Criteria criteria, String displayName) {
            return registerNewObjective(name, criteria, displayName, criteria.getDefaultRenderType());
        }

        @Override
        public Objective registerNewObjective(String name, Criteria criteria, String displayName, RenderType renderType) {
            if (objectives.containsKey(name)) throw new IllegalArgumentException("Ziel '" + name + "' gibt es schon");
            Obj o = new Obj(this, name, criteria, displayName == null ? name : displayName, renderType);
            objectives.put(name, o);
            send("sb_objective", "name", name, "mode", 0, "title", o.display, "hearts", renderType == RenderType.HEARTS);
            return o;
        }

        @Override public Objective getObjective(String name) { return objectives.get(name); }

        @Override public Objective getObjective(DisplaySlot slot) { return slots.get(slot); }

        @Override public Set<Objective> getObjectives() { return new LinkedHashSet<>(objectives.values()); }

        @Override
        public Set<Objective> getObjectivesByCriteria(String criteria) {
            Set<Objective> out = new LinkedHashSet<>();
            for (Obj o : objectives.values()) if (o.criteria.getName().equals(criteria)) out.add(o);
            return out;
        }

        @Override
        public void clearSlot(DisplaySlot slot) {
            if (slots.remove(slot) != null) send("sb_display", "slot", slot.getId(), "name", "");
        }

        @Override
        public Team registerNewTeam(String name) {
            if (teams.containsKey(name)) throw new IllegalArgumentException("Team '" + name + "' gibt es schon");
            TeamImpl t = new TeamImpl(this, name);
            teams.put(name, t);
            for (Player p : viewers()) t.sendCreate(p);
            return t;
        }

        @Override public Team getTeam(String name) { return teams.get(name); }

        @Override public Set<Team> getTeams() { return new LinkedHashSet<>(teams.values()); }

        @Override
        public Team getEntryTeam(String entry) {
            for (TeamImpl t : teams.values()) if (t.entries.contains(entry)) return t;
            return null;
        }

        @Override
        public Set<Score> getScores(String entry) {
            Set<Score> out = new LinkedHashSet<>();
            for (Obj o : objectives.values()) if (o.scores.containsKey(entry)) out.add(o.scores.get(entry));
            return out;
        }

        @Override
        public void resetScores(String entry) {
            for (Obj o : objectives.values()) o.scores.remove(entry);
            send("sb_reset", "entry", entry, "objective", null);
        }

        @Override
        public Set<String> getEntries() {
            Set<String> out = new LinkedHashSet<>();
            for (Obj o : objectives.values()) out.addAll(o.scores.keySet());
            return out;
        }
    }

    final class Obj implements Objective {
        final Board board;
        final String name;
        final Criteria criteria;
        String display;
        RenderType render;
        final Map<String, ScoreImpl> scores = new LinkedHashMap<>();
        boolean gone;

        Obj(Board b, String name, Criteria c, String display, RenderType render) {
            this.board = b;
            this.name = name;
            this.criteria = c;
            this.display = display;
            this.render = render;
        }

        private void check() { if (gone) throw new IllegalStateException("Ziel wurde entfernt"); }

        private void update() { board.send("sb_objective", "name", name, "mode", 2, "title", display, "hearts", render == RenderType.HEARTS); }

        @Override public String getName() { return name; }

        @Override public String getDisplayName() { return display; }

        @Override
        public void setDisplayName(String n) {
            check();
            display = n == null ? name : n;
            update();
        }

        @Override public String getCriteria() { return criteria.getName(); }

        @Override public Criteria getTrackedCriteria() { return criteria; }

        @Override public boolean isModifiable() { return !criteria.isReadOnly(); }

        @Override public Scoreboard getScoreboard() { return gone ? null : board; }

        @Override
        public void unregister() {
            check();
            gone = true;
            board.objectives.remove(name);
            board.slots.values().removeIf(o -> o == this);
            board.send("sb_objective", "name", name, "mode", 1);
        }

        @Override
        public void setDisplaySlot(DisplaySlot slot) {
            check();
            board.slots.values().removeIf(o -> o == this);
            if (slot == null) {
                board.send("sb_display", "slot", 1, "name", "");
                return;
            }
            board.slots.put(slot, this);
            board.send("sb_display", "slot", slot.getId(), "name", name);
        }

        @Override
        public DisplaySlot getDisplaySlot() {
            for (Map.Entry<DisplaySlot, Obj> e : board.slots.entrySet()) if (e.getValue() == this) return e.getKey();
            return null;
        }

        @Override
        public void setRenderType(RenderType type) {
            check();
            render = type;
            update();
        }

        @Override public RenderType getRenderType() { return render; }

        @Override
        public Score getScore(String entry) {
            check();
            return scores.containsKey(entry) ? scores.get(entry) : new ScoreImpl(this, entry);
        }
    }

    final class ScoreImpl implements Score {
        final Obj obj;
        final String entry;
        int value;
        String custom;

        ScoreImpl(Obj obj, String entry) {
            this.obj = obj;
            this.entry = entry;
        }

        private void push() {
            obj.scores.put(entry, this);
            obj.board.send("sb_score", "entry", entry, "objective", obj.name, "value", value, "display", custom);
        }

        @Override public String getEntry() { return entry; }

        @Override public Objective getObjective() { return obj; }

        @Override public int getScore() { return obj.scores.containsKey(entry) ? obj.scores.get(entry).value : 0; }

        @Override
        public void setScore(int score) {
            value = score;
            push();
        }

        @Override public boolean isScoreSet() { return obj.scores.containsKey(entry); }

        @Override
        public void resetScore() {
            obj.scores.remove(entry);
            obj.board.send("sb_reset", "entry", entry, "objective", obj.name);
        }

        @Override public Scoreboard getScoreboard() { return obj.board; }

        @Override
        public void customName(Component name) {
            custom = name == null ? null : name.toLegacy();
            if (isScoreSet()) push();
        }

        @Override public Component customName() { return custom == null ? null : Component.text(custom); }
    }

    final class TeamImpl implements Team {
        final Board board;
        final String name;
        String display, prefix = "", suffix = "";
        ChatColor color;
        boolean friendlyFire = true, seeInvisibles = true, gone;
        final Map<Option, OptionStatus> options = new LinkedHashMap<>();
        final Set<String> entries = new LinkedHashSet<>();

        TeamImpl(Board b, String name) {
            this.board = b;
            this.name = name;
            this.display = name;
        }

        private int flags() { return (friendlyFire ? 1 : 0) | (seeInvisibles ? 2 : 0); }

        private Object[] info(int mode) {
            return new Object[]{"name", name, "mode", mode, "display", display, "prefix", prefix, "suffix", suffix, "color", color(color),
                    "flags", flags(), "nametag", rule(getOption(Option.NAME_TAG_VISIBILITY), false),
                    "collision", rule(getOption(Option.COLLISION_RULE), true), "entries", new ArrayList<>(entries)};
        }

        void sendCreate(Player p) { board.sendTo(p, "sb_team", info(0)); }

        private void update() {
            if (gone) throw new IllegalStateException("Team wurde entfernt");
            board.send("sb_team", info(2));
        }

        @Override public String getName() { return name; }

        @Override public String getDisplayName() { return display; }

        @Override public void setDisplayName(String n) { display = n == null ? name : n; update(); }

        @Override public String getPrefix() { return prefix; }

        @Override public void setPrefix(String p) { prefix = p == null ? "" : p; update(); }

        @Override public String getSuffix() { return suffix; }

        @Override public void setSuffix(String s) { suffix = s == null ? "" : s; update(); }

        @Override public ChatColor getColor() { return color == null ? ChatColor.RESET : color; }

        @Override public void setColor(ChatColor c) { color = c; update(); }

        @Override public boolean allowFriendlyFire() { return friendlyFire; }

        @Override public void setAllowFriendlyFire(boolean e) { friendlyFire = e; update(); }

        @Override public boolean canSeeFriendlyInvisibles() { return seeInvisibles; }

        @Override public void setCanSeeFriendlyInvisibles(boolean e) { seeInvisibles = e; update(); }

        @Override public OptionStatus getOption(Option o) { return options.getOrDefault(o, OptionStatus.ALWAYS); }

        @Override public void setOption(Option o, OptionStatus s) { options.put(o, s); update(); }

        @Override public Set<String> getEntries() { return new HashSet<>(entries); }

        @Override public int getSize() { return entries.size(); }

        @Override public Scoreboard getScoreboard() { return gone ? null : board; }

        @Override
        public void addEntry(String entry) {
            for (TeamImpl t : board.teams.values()) if (t != this && t.entries.contains(entry)) t.removeEntry(entry);   // nur ein Team
            if (entries.add(entry)) board.send("sb_team", "name", name, "mode", 3, "entries", List.of(entry));
        }

        @Override
        public boolean removeEntry(String entry) {
            if (!entries.remove(entry)) return false;
            board.send("sb_team", "name", name, "mode", 4, "entries", List.of(entry));
            return true;
        }

        @Override public boolean hasEntry(String entry) { return entries.contains(entry); }

        @Override
        public void unregister() {
            gone = true;
            board.teams.remove(name);
            board.send("sb_team", "name", name, "mode", 1);
        }
    }
}
