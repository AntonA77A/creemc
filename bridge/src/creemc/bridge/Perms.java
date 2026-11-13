package creemc.bridge;

import java.util.Map;
import org.bukkit.plugin.Plugin;

/** Rechte wie Bukkit ohne Rechte-Plugin: Standardwert aus plugin.yml ("true", "false", "op", "not op"), sonst nur OPs. */
final class Perms {
    private Perms() { }

    static boolean has(boolean op, String perm) {
        if (perm == null || perm.isEmpty()) return true;
        String def = defaultOf(perm.toLowerCase());
        if (def == null) return op;
        return switch (def.toLowerCase()) {
            case "true" -> true;
            case "false" -> false;
            case "not op", "!op", "notop", "!operator", "not operator", "non-op" -> !op;
            default -> op;
        };
    }

    static boolean isSet(String perm) { return defaultOf(perm.toLowerCase()) != null; }

    private static String defaultOf(String perm) {
        Bridge b = Bridge.get();
        if (b == null) return null;
        for (Plugin p : b.plugins) {
            String d = find(p.getDescription().getPermissionsRaw(), perm);
            if (d != null) return d;
        }
        return null;
    }

    private static String find(Map<String, Object> perms, String perm) {
        for (Map.Entry<String, Object> e : perms.entrySet()) {
            if (!(e.getValue() instanceof Map<?, ?> m)) continue;
            if (e.getKey().equalsIgnoreCase(perm)) return m.get("default") == null ? "op" : String.valueOf(m.get("default"));
            // Kinder ("children: {a.b: true}") erben den Standard des Elternrechts
            if (m.get("children") instanceof Map<?, ?> ch) {
                for (Map.Entry<?, ?> c : ch.entrySet())
                    if (String.valueOf(c.getKey()).equalsIgnoreCase(perm) && Boolean.TRUE.equals(c.getValue()) && m.get("default") != null)
                        return String.valueOf(m.get("default"));
            }
        }
        return null;
    }
}
