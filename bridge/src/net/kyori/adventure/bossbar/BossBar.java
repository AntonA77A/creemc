package net.kyori.adventure.bossbar;

import java.util.EnumSet;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.UUID;
import java.util.function.Consumer;
import net.kyori.adventure.text.Component;

/** Adventure-Bossbar (Paper): player.showBossBar(bar) / hideBossBar(bar). */
public final class BossBar {
    public enum Color { PINK, BLUE, RED, GREEN, YELLOW, PURPLE, WHITE }

    public enum Overlay { PROGRESS, NOTCHED_6, NOTCHED_10, NOTCHED_12, NOTCHED_20 }

    public enum Flag { DARKEN_SCREEN, PLAY_BOSS_MUSIC, CREATE_WORLD_FOG }

    public static final float MIN_PROGRESS = 0f, MAX_PROGRESS = 1f;

    private final UUID id = UUID.randomUUID();
    private Component name;
    private float progress;
    private Color color;
    private Overlay overlay;
    private final Set<Flag> flags = EnumSet.noneOf(Flag.class);
    /** Wer die Bossbar sieht (Player-UUIDs); wird von CreeMC gepflegt. */
    private final Set<UUID> viewers = new LinkedHashSet<>();
    private static Consumer<Object[]> sender = a -> { };

    private BossBar(Component name, float progress, Color color, Overlay overlay, Set<Flag> flags) {
        this.name = name;
        this.progress = progress;
        this.color = color;
        this.overlay = overlay;
        this.flags.addAll(flags);
    }

    public static BossBar bossBar(Component name, float progress, Color color, Overlay overlay) {
        return new BossBar(name, progress, color, overlay, Set.of());
    }

    public static BossBar bossBar(Component name, float progress, Color color, Overlay overlay, Set<Flag> flags) {
        return new BossBar(name, progress, color, overlay, flags);
    }

    /** CreeMC intern: Paketversand (UUID der Bar, Spieler-UUID, Aktion). */
    public static void creeSender(Consumer<Object[]> s) { sender = s; }

    public UUID creeId() { return id; }

    public Set<UUID> creeViewers() { return viewers; }

    public int creeFlags() {
        int f = 0;
        if (flags.contains(Flag.DARKEN_SCREEN)) f |= 1;
        if (flags.contains(Flag.PLAY_BOSS_MUSIC)) f |= 2;
        if (flags.contains(Flag.CREATE_WORLD_FOG)) f |= 4;
        return f;
    }

    private void update(int action) {
        for (UUID v : viewers) sender.accept(new Object[]{this, v, action});
    }

    public Component name() { return name; }

    public BossBar name(Component n) { name = n; update(3); return this; }

    public float progress() { return progress; }

    public BossBar progress(float p) { progress = Math.max(0f, Math.min(1f, p)); update(2); return this; }

    public Color color() { return color; }

    public BossBar color(Color c) { color = c; update(4); return this; }

    public Overlay overlay() { return overlay; }

    public BossBar overlay(Overlay o) { overlay = o; update(4); return this; }

    public Set<Flag> flags() { return EnumSet.copyOf(flags.isEmpty() ? EnumSet.noneOf(Flag.class) : flags); }

    public BossBar addFlag(Flag f) { flags.add(f); update(5); return this; }

    public BossBar removeFlag(Flag f) { flags.remove(f); update(5); return this; }

    public boolean hasFlag(Flag f) { return flags.contains(f); }
}