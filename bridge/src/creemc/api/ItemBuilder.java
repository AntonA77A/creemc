package creemc.api;

import java.util.Map;
import java.util.TreeMap;

/**
 * Baut CreeMC-Items mit Verzauberungen und eigenem Namen.
 * Ergebnis ist ein Item-Text wie "minecraft:diamond_sword@{"ench":{"minecraft:sharpness":5}}",
 * den alle CreeMC-Funktionen (give, dropItem ...) verstehen.
 */
public final class ItemBuilder {
    private final String item;
    private final Map<String, Integer> ench = new TreeMap<>();
    private final Map<String, Integer> stored = new TreeMap<>();
    private String name;

    ItemBuilder(String item) { this.item = item.contains(":") ? item : "minecraft:" + item; }

    /** Verzauberung hinzufügen, z.B. enchant("sharpness", 5). Bei Büchern: gespeicherte Verzauberung. */
    public ItemBuilder enchant(String enchantment, int level) {
        String e = enchantment.contains(":") ? enchantment : "minecraft:" + enchantment;
        if (item.equals("minecraft:enchanted_book")) stored.put(e, level);
        else ench.put(e, level);
        return this;
    }

    /** Eigener Name (Farbcodes mit &amp;). */
    public ItemBuilder name(String name) {
        this.name = name == null ? null : CreeMC.color(name);
        return this;
    }

    /** Fertiger Item-Text. */
    public String build() {
        // Schlüssel sortiert und kompakt, damit gleiche Items stapeln (wie im Server)
        StringBuilder sb = new StringBuilder();
        if (!ench.isEmpty()) sb.append("\"ench\":").append(obj(ench));
        if (name != null && !name.isEmpty()) {
            if (sb.length() > 0) sb.append(',');
            sb.append("\"name\":").append(str(name));
        }
        if (!stored.isEmpty()) {
            if (sb.length() > 0) sb.append(',');
            sb.append("\"stored\":").append(obj(stored));
        }
        return sb.length() == 0 ? item : item + "@{" + sb + "}";
    }

    @Override
    public String toString() { return build(); }

    private static String obj(Map<String, Integer> m) {
        StringBuilder sb = new StringBuilder("{");
        for (var e : m.entrySet()) {
            if (sb.length() > 1) sb.append(',');
            sb.append(str(e.getKey())).append(':').append(e.getValue());
        }
        return sb.append('}').toString();
    }

    private static String str(String s) {
        StringBuilder sb = new StringBuilder("\"");
        for (char c : s.toCharArray()) {
            if (c == '"' || c == '\\') sb.append('\\').append(c);
            else if (c < 0x20) sb.append(String.format("\\u%04x", (int) c));
            else sb.append(c);
        }
        return sb.append('"').toString();
    }
}
