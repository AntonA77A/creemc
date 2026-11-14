package creemc.api;

import creemc.bridge.Bridge;
import creemc.bridge.Json;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.bukkit.Location;
import org.bukkit.entity.Player;

/**
 * Die eigene CreeMC-API: alles, was es in Bukkit/Paper nicht (so einfach) gibt.
 * Funktioniert in CreeMC-Plugins ({@link CreeMCPlugin}) und in normalen Bukkit-Plugins.
 *
 * <pre>
 * CreeMC.explode(x, y, z, 4, true);
 * CreeMC.spawnMob("zombie", x, y, z);
 * CreeMC.setGameRule("fallDamage", "false");
 * CreeMC.give(player, CreeMC.item("diamond_sword").enchant("sharpness", 5).name("&amp;6Klinge"), 1);
 * </pre>
 *
 * Farbcodes mit &amp; oder §. Block-Namen wie in Vanilla, auch mit Zuständen: "lever[powered=true]".
 */
public final class CreeMC {
    private CreeMC() { }

    /** API-Version (steigt, wenn neue Funktionen dazukommen). */
    public static final int API_VERSION = 2;

    private static Bridge b() { return Bridge.get(); }
    private static String uuid(Player p) { return p.getUniqueId().toString(); }
    private static String col(String s) { return color(s); }

    /** &amp;-Farbcodes in §-Codes umwandeln ("&amp;aHallo" -&gt; grünes Hallo). "&amp;&amp;" bleibt ein &amp;. */
    public static String color(String s) {
        if (s == null) return "";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '&' && i + 1 < s.length()) {
                char n = Character.toLowerCase(s.charAt(i + 1));
                if (n == '&') { sb.append('&'); i++; continue; }
                if ("0123456789abcdefklmnor".indexOf(n) >= 0) { sb.append('§').append(n); i++; continue; }
            }
            sb.append(c);
        }
        return sb.toString();
    }

    // ---- Server ----------------------------------------------------------------

    /** Aktueller Server-Tick (20 pro Sekunde). */
    public static long tick() { return (long) Json.num(b().request("tick"), 0); }

    /** Nachricht an alle Spieler (und die Konsole). */
    public static void broadcast(String message) { b().call("broadcast", "text", col(message)); }

    /** Führt einen Befehl als Konsole aus, z.B. "time set 0". */
    public static void console(String command) { b().call("command", "line", command); }

    /** Spieler mit diesem Namen oder null. */
    public static Player player(String name) { return org.bukkit.Bukkit.getPlayerExact(name); }

    // ---- Spielregeln -----------------------------------------------------------

    /** Wert einer Spielregel wie in Vanilla ("fallDamage", "keepInventory", "doDaylightCycle" ...) oder null. */
    public static String gameRule(String rule) {
        Object v = b().request("gamerule", "rule", rule);
        return v == null ? null : String.valueOf(v);
    }

    /** Spielregel setzen (wie /gamerule). -> false, wenn es die Regel nicht gibt oder der Wert ungültig ist. */
    public static boolean setGameRule(String rule, String value) {
        return Boolean.TRUE.equals(b().request("set_gamerule", "rule", rule, "value", value));
    }

    /** Alle Spielregeln mit ihren Werten. */
    public static Map<String, String> gameRules() {
        Map<String, String> out = new LinkedHashMap<>();
        Object v = b().request("gamerules");
        if (v instanceof Map<?, ?> m) m.forEach((k, val) -> out.put(String.valueOf(k), String.valueOf(val)));
        return out;
    }

    // ---- Welt ------------------------------------------------------------------

    /** Block an x, y, z, z.B. "minecraft:oak_stairs[facing=north,half=bottom,...]". */
    public static String block(int x, int y, int z) { return String.valueOf(b().request("block", "x", x, "y", y, "z", z)); }

    /** Block setzen (mit Nachbar-Updates wie in Vanilla: Redstone, Wasser, Sand ...). */
    public static void setBlock(int x, int y, int z, String block) { b().call("set_block", "x", x, "y", y, "z", z, "block", block); }

    /** Block abbauen; drops = true lässt die Items wie beim Abbauen fallen. */
    public static void breakBlock(int x, int y, int z, boolean drops) {
        b().call("break_block", "x", x, "y", y, "z", z, "drops", drops);
    }

    /** Höchster Block (nicht Luft) an x, z. */
    public static int highestY(int x, int z) { return (int) Json.num(b().request("highest", "x", x, "z", z), -64); }

    /** Explosion wie TNT (TNT hat Stärke 4, Creeper 3). */
    public static void explode(double x, double y, double z, double power, boolean breakBlocks) {
        b().call("explode", "x", x, "y", y, "z", z, "power", power, "break", breakBlocks);
    }

    /** Angezündetes TNT erzeugen (fuse = Ticks bis zur Explosion, Vanilla 80). */
    public static void tnt(double x, double y, double z, int fuse) { b().call("tnt", "x", x, "y", y, "z", z, "fuse", fuse); }

    /** Mob erzeugen: pig, cow, sheep, chicken, zombie, spider, creeper. -> Entity-ID oder -1. */
    public static int spawnMob(String type, double x, double y, double z) {
        return (int) Json.num(b().request("spawn_mob", "type", type, "x", x, "y", y, "z", z), -1);
    }

    /** Mob-Typen, die CreeMC kennt. */
    public static List<String> mobTypes() {
        List<String> out = new ArrayList<>();
        for (Object o : Json.list(b().request("mob_types"))) out.add(String.valueOf(o));
        return out;
    }

    /** Item in die Welt fallen lassen (auch Items aus {@link #item(String)}). */
    public static void dropItem(double x, double y, double z, String item, int count) {
        b().call("drop_item", "x", x, "y", y, "z", z, "item", item, "count", count);
    }
    public static void dropItem(double x, double y, double z, ItemBuilder item, int count) { dropItem(x, y, z, item.build(), count); }

    /** Sound an einer Stelle für alle in der Nähe, z.B. "entity.player.levelup". */
    public static void sound(double x, double y, double z, String sound, float volume, float pitch) {
        b().call("sound", "name", sound.contains(":") ? sound : "minecraft:" + sound, "x", x, "y", y, "z", z, "volume", volume, "pitch", pitch);
    }

    /** Uhrzeit der Welt setzen (0 = Morgen, 6000 = Mittag, 13000 = Nacht). */
    public static void time(long time) { b().call("time", "value", time); }

    // ---- Spieler ---------------------------------------------------------------

    /** Voll heilen (Leben, Hunger, Feuer). */
    public static void heal(Player p) { b().call("heal", "to", uuid(p)); }

    /** Schaden mit Ursache wie in Vanilla (Rüstung zählt): "attack", "fall", "fire", "lava", "drown", "explosion", "generic". */
    public static void damage(Player p, double amount, String cause) { b().call("damage_cause", "to", uuid(p), "amount", amount, "cause", cause); }

    /** Spieler töten (mit eigener Todesnachricht, null = Standard). */
    public static void kill(Player p, String message) { b().call("kill", "to", uuid(p), "message", message == null ? "" : col(message)); }

    /** Erfahrungspunkte geben (mit Sound, Stufenaufstieg wie Vanilla). */
    public static void giveXp(Player p, int points) { b().call("give_xp_sound", "to", uuid(p), "value", points); }

    /** Titel in der Bildschirmmitte. */
    public static void title(Player p, String title, String subtitle) {
        b().call("title", "to", uuid(p), "title", col(title), "sub", col(subtitle), "fade_in", 10, "stay", 70, "fade_out", 20);
    }

    /** Text über der Hotbar. */
    public static void actionBar(Player p, String text) { b().call("actionbar", "to", uuid(p), "text", col(text)); }

    /** Kopf- und Fußzeile der Tab-Liste. */
    public static void tabList(Player p, String header, String footer) {
        b().call("tablist", "to", uuid(p), "header", col(header), "footer", col(footer));
    }

    /** Item geben (mit Verzauberungen/Namen aus {@link #item(String)}). -> Anzahl, die nicht gepasst hat. */
    public static int give(Player p, String item, int count) {
        return (int) Json.num(b().request("give", "uuid", uuid(p), "item", item.contains(":") ? item : "minecraft:" + item, "count", count), count);
    }
    public static int give(Player p, ItemBuilder item, int count) { return give(p, item.build(), count); }

    /** Items wegnehmen. -> Anzahl, die gefehlt hat. */
    public static int take(Player p, String item, int count) {
        return (int) Json.num(b().request("take", "uuid", uuid(p), "item", item.contains(":") ? item : "minecraft:" + item, "count", count), count);
    }

    /** Spieler wegstoßen (Geschwindigkeit in Blöcken pro Tick). */
    public static void push(Player p, double vx, double vy, double vz) { b().call("velocity", "to", uuid(p), "x", vx, "y", vy, "z", vz); }

    /** Teleportieren. */
    public static void teleport(Player p, double x, double y, double z) { b().call("teleport", "to", uuid(p), "x", x, "y", y, "z", z); }
    public static void teleport(Player p, Location l) { teleport(p, l.getX(), l.getY(), l.getZ()); }

    /** Spieler-Infos vom Server (Leben, Hunger, Position ...) als Map. */
    public static Map<String, Object> info(UUID player) {
        Object v = b().request("player_info", "uuid", player.toString());
        return v instanceof Map<?, ?> ? Json.obj(v) : Map.of();
    }

    // ---- Items -----------------------------------------------------------------

    /** Item mit Verzauberungen und Namen bauen: CreeMC.item("diamond_sword").enchant("sharpness", 5).name("&amp;bKlinge"). */
    public static ItemBuilder item(String item) { return new ItemBuilder(item); }

    /** ItemBuilder als Bukkit-ItemStack (für Inventare, GUIs). */
    public static org.bukkit.inventory.ItemStack stack(ItemBuilder item, int count) {
        return org.bukkit.inventory.ItemStack.fromCree(item.build(), count);
    }

    // ---- GUIs, Seitenleiste, Bossbar, Partikel ---------------------------------

    /** Menü-GUI mit Klick-Aktionen (rows = 1 bis 6). In einem CreeMCPlugin einfacher: gui(titel, reihen). */
    public static Gui gui(org.bukkit.plugin.Plugin plugin, String title, int rows) { return new Gui(plugin, title, rows); }

    /** Seitenleiste für einen Spieler: CreeMC.sidebar(p, "&amp;6Titel").lines("Zeile 1", "Zeile 2"). */
    public static Sidebar sidebar(Player p, String title) { return new Sidebar(p, title); }

    /** Bossbar für einen Spieler zeigen; Farbe: pink, blue, red, green, yellow, purple, white. -> zum späteren Ändern/Entfernen. */
    public static org.bukkit.boss.BossBar bossBar(Player p, String text, double progress, String color) {
        org.bukkit.boss.BarColor c;
        try {
            c = org.bukkit.boss.BarColor.valueOf(color == null ? "PINK" : color.toUpperCase());
        } catch (IllegalArgumentException e) {
            c = org.bukkit.boss.BarColor.PINK;
        }
        org.bukkit.boss.BossBar bar = org.bukkit.Bukkit.createBossBar(color(text), c, org.bukkit.boss.BarStyle.SOLID);
        bar.setProgress(Math.max(0, Math.min(1, progress)));
        bar.addPlayer(p);
        return bar;
    }

    /** Partikel für alle in der Nähe, z.B. particle("heart", x, y, z, 5). */
    public static void particle(String name, double x, double y, double z, int count) {
        b().call("particle", "name", name.contains(":") ? name : "minecraft:" + name, "x", x, "y", y, "z", z, "dx", 0.3, "dy", 0.3,
                "dz", 0.3, "speed", 0.0, "count", count);
    }

    /** Farbiger Staub (rgb wie 0xFF0000 = rot). */
    public static void dust(double x, double y, double z, int rgb, float size, int count) {
        b().call("particle", "name", "minecraft:dust", "x", x, "y", y, "z", z, "dx", 0.2, "dy", 0.2, "dz", 0.2, "speed", 0.0,
                "count", count, "color", rgb, "size", (double) size);
    }

    /** Partikel-Namen, die der Server kennt. */
    public static List<String> particles() {
        List<String> out = new ArrayList<>();
        for (Object o : Json.list(b().request("particles"))) out.add(String.valueOf(o));
        return out;
    }

    /** Fliegen erlauben (auch im Überleben). */
    public static void allowFlight(Player p, boolean allow) { p.setAllowFlight(allow); }

    /** Spieler für einen anderen unsichtbar machen (auch aus der Tab-Liste). */
    public static void hide(Player viewer, Player target) { viewer.hidePlayer(null, target); }

    public static void show(Player viewer, Player target) { viewer.showPlayer(null, target); }
}
