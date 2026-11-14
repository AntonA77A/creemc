package creemc.bridge;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.bukkit.boss.BarColor;
import org.bukkit.boss.BarFlag;
import org.bukkit.boss.BarStyle;
import org.bukkit.boss.BossBar;
import org.bukkit.entity.Player;

/** Bukkit-Bossbar: schickt die Vanilla-Pakete an alle Spieler, die sie sehen. */
final class BossBars {
    private BossBars() { }

    static void send(Player p, UUID id, int action, String title, double progress, int color, int style, int flags) {
        Bridge.get().call("bossbar", "to", p.getUniqueId().toString(), "id", id.toString(), "action", action, "title", title,
                "progress", progress, "color", color, "style", style, "flags", flags);
    }

    static final class Impl implements BossBar {
        final UUID id = UUID.randomUUID();
        String title;
        BarColor color;
        BarStyle style;
        final Set<BarFlag> flags = EnumSet.noneOf(BarFlag.class);
        double progress = 1.0;
        boolean visible = true;
        final Set<Player> players = new LinkedHashSet<>();

        Impl(String title, BarColor color, BarStyle style, BarFlag... flags) {
            this.title = title == null ? "" : title;
            this.color = color == null ? BarColor.PINK : color;
            this.style = style == null ? BarStyle.SOLID : style;
            this.flags.addAll(List.of(flags));
        }

        private int flagBits() {
            int f = 0;
            if (flags.contains(BarFlag.DARKEN_SKY)) f |= 1;
            if (flags.contains(BarFlag.PLAY_BOSS_MUSIC)) f |= 2;
            if (flags.contains(BarFlag.CREATE_FOG)) f |= 4;
            return f;
        }

        private void add(Player p) { send(p, id, 0, title, progress, color.ordinal(), style.ordinal(), flagBits()); }

        private void all(int action) {
            if (!visible) return;
            for (Player p : players) send(p, id, action, title, progress, color.ordinal(), style.ordinal(), flagBits());
        }

        @Override public String getTitle() { return title; }

        @Override public void setTitle(String t) { title = t == null ? "" : t; all(3); }

        @Override public BarColor getColor() { return color; }

        @Override public void setColor(BarColor c) { color = c; all(4); }

        @Override public BarStyle getStyle() { return style; }

        @Override public void setStyle(BarStyle s) { style = s; all(4); }

        @Override public void removeFlag(BarFlag f) { flags.remove(f); all(5); }

        @Override public void addFlag(BarFlag f) { flags.add(f); all(5); }

        @Override public boolean hasFlag(BarFlag f) { return flags.contains(f); }

        @Override
        public void setProgress(double p) {
            if (p < 0 || p > 1) throw new IllegalArgumentException("Fortschritt muss zwischen 0 und 1 liegen: " + p);
            progress = p;
            all(2);
        }

        @Override public double getProgress() { return progress; }

        @Override
        public void addPlayer(Player p) {
            if (players.add(p) && visible) add(p);
        }

        @Override
        public void removePlayer(Player p) {
            if (players.remove(p) && visible) send(p, id, 1, title, progress, 0, 0, 0);
        }

        @Override
        public void removeAll() {
            for (Player p : new ArrayList<>(players)) removePlayer(p);
        }

        @Override public List<Player> getPlayers() { return new ArrayList<>(players); }

        @Override
        public void setVisible(boolean v) {
            if (v == visible) return;
            visible = v;
            for (Player p : players) {
                if (v) add(p);
                else send(p, id, 1, title, progress, 0, 0, 0);
            }
        }

        @Override public boolean isVisible() { return visible; }
    }
}