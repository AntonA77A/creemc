package creemc.bridge;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** Einfacher YAML-Leser/-Schreiber für plugin.yml und config.yml (Maps, Listen, Werte, Kommentare). */
public final class Yaml {
    private final List<String[]> lines = new ArrayList<>(); // {Einrückung, Inhalt}
    private int pos;

    private Yaml(String text) {
        for (String raw : text.replace("\t", "  ").split("\r?\n")) {
            String noComment = stripComment(raw);
            if (noComment.isBlank() || noComment.trim().equals("---")) continue;
            int ind = 0;
            while (ind < noComment.length() && noComment.charAt(ind) == ' ') ind++;
            lines.add(new String[]{String.valueOf(ind), noComment.substring(ind).stripTrailing()});
        }
    }

    public static Map<String, Object> load(String text) {
        Yaml y = new Yaml(text);
        Object o = y.block(0);
        return o instanceof Map ? Json.obj(o) : new LinkedHashMap<>();
    }

    private static String stripComment(String l) {
        boolean sq = false, dq = false;
        for (int k = 0; k < l.length(); k++) {
            char c = l.charAt(k);
            if (c == '\'' && !dq) sq = !sq;
            else if (c == '"' && !sq) dq = !dq;
            else if (c == '#' && !sq && !dq && (k == 0 || l.charAt(k - 1) == ' ')) return l.substring(0, k);
        }
        return l;
    }

    private int indent() { return Integer.parseInt(lines.get(pos)[0]); }

    private String text() { return lines.get(pos)[1]; }

    private Object block(int min) {
        if (pos >= lines.size()) return null;
        int ind = indent();
        if (ind < min) return null;
        if (text().startsWith("- ") || text().equals("-")) {
            List<Object> l = new ArrayList<>();
            while (pos < lines.size() && indent() == ind && (text().startsWith("- ") || text().equals("-"))) {
                String rest = text().length() > 1 ? text().substring(2).trim() : "";
                if (rest.isEmpty()) {
                    pos++;
                    l.add(block(ind + 1));
                } else if (keyIndex(rest) > 0) {
                    // "- name: x" -> Map, weitere Schlüssel tiefer eingerückt
                    lines.set(pos, new String[]{String.valueOf(ind + 2), rest});
                    l.add(block(ind + 2));
                } else {
                    pos++;
                    l.add(scalar(rest));
                }
            }
            return l;
        }
        Map<String, Object> m = new LinkedHashMap<>();
        while (pos < lines.size() && indent() == ind) {
            String t = text();
            int k = keyIndex(t);
            if (k < 0) { pos++; continue; }
            String key = unquote(t.substring(0, k).trim());
            String rest = t.substring(k + 1).trim();
            pos++;
            if (rest.isEmpty()) {
                Object child = pos < lines.size() && indent() > ind ? block(ind + 1)
                        : (pos < lines.size() && indent() == ind && text().startsWith("- ") ? block(ind) : null);
                m.put(key, child);
            } else if (rest.equals("|") || rest.equals(">") || rest.equals("|-") || rest.equals(">-")) {
                StringBuilder b = new StringBuilder();
                while (pos < lines.size() && indent() > ind) {
                    if (b.length() > 0) b.append(rest.startsWith("|") ? "\n" : " ");
                    b.append(text());
                    pos++;
                }
                m.put(key, b.toString());
            } else m.put(key, scalar(rest));
        }
        return m;
    }

    private static int keyIndex(String t) {
        boolean sq = false, dq = false;
        for (int k = 0; k < t.length(); k++) {
            char c = t.charAt(k);
            if (c == '\'' && !dq) sq = !sq;
            else if (c == '"' && !sq) dq = !dq;
            else if (c == ':' && !sq && !dq && (k + 1 == t.length() || t.charAt(k + 1) == ' ')) return k;
        }
        return -1;
    }

    private static String unquote(String v) {
        if (v.length() >= 2 && ((v.startsWith("\"") && v.endsWith("\"")) || (v.startsWith("'") && v.endsWith("'")))) {
            String in = v.substring(1, v.length() - 1);
            return v.startsWith("'") ? in.replace("''", "'") : in.replace("\\\"", "\"").replace("\\n", "\n");
        }
        return v;
    }

    static Object scalar(String v) {
        if (v.startsWith("[") && v.endsWith("]")) {
            List<Object> l = new ArrayList<>();
            String in = v.substring(1, v.length() - 1).trim();
            if (!in.isEmpty()) for (String part : in.split(",")) l.add(scalar(part.trim()));
            return l;
        }
        if (v.startsWith("{") && v.endsWith("}")) {
            Map<String, Object> m = new LinkedHashMap<>();
            String in = v.substring(1, v.length() - 1).trim();
            if (!in.isEmpty()) for (String part : in.split(",")) {
                int k = part.indexOf(':');
                if (k > 0) m.put(unquote(part.substring(0, k).trim()), scalar(part.substring(k + 1).trim()));
            }
            return m;
        }
        if (v.startsWith("\"") || v.startsWith("'")) return unquote(v);
        switch (v) {
            case "true", "yes", "on", "True", "TRUE" -> { return Boolean.TRUE; }
            case "false", "no", "off", "False", "FALSE" -> { return Boolean.FALSE; }
            case "null", "~" -> { return null; }
        }
        try {
            if (v.matches("-?\\d+")) {
                long l = Long.parseLong(v);
                return l >= Integer.MIN_VALUE && l <= Integer.MAX_VALUE ? (Object) (int) l : (Object) l;
            }
            if (v.matches("-?\\d*\\.\\d+([eE][-+]?\\d+)?")) return Double.parseDouble(v);
        } catch (NumberFormatException ignored) {
        }
        return v;
    }

    // ---- Schreiben ----

    public static String dump(Map<String, Object> m) {
        StringBuilder b = new StringBuilder();
        dumpMap(b, m, 0);
        return b.toString();
    }

    private static void dumpMap(StringBuilder b, Map<?, ?> m, int ind) {
        for (Map.Entry<?, ?> e : m.entrySet()) {
            b.append(" ".repeat(ind)).append(key(String.valueOf(e.getKey()))).append(':');
            Object v = e.getValue();
            if (v instanceof Map<?, ?> sub && !sub.isEmpty()) {
                b.append('\n');
                dumpMap(b, sub, ind + 2);
            } else if (v instanceof List<?> l && !l.isEmpty()) {
                b.append('\n');
                for (Object x : l) {
                    if (x instanceof Map<?, ?> xm) {
                        b.append(" ".repeat(ind)).append("-\n");
                        dumpMap(b, xm, ind + 2);
                    } else b.append(" ".repeat(ind)).append("- ").append(val(x)).append('\n');
                }
            } else b.append(' ').append(v instanceof Map ? "{}" : v instanceof List ? "[]" : val(v)).append('\n');
        }
    }

    private static String key(String k) { return k.matches("[A-Za-z0-9_.\\-]+") ? k : "'" + k.replace("'", "''") + "'"; }

    private static String val(Object v) {
        if (v == null) return "null";
        if (v instanceof Number || v instanceof Boolean) return v.toString();
        String s = v.toString();
        if (s.isEmpty() || !s.matches("[A-Za-z0-9_./ \\-]*[A-Za-z0-9_./\\-]") || scalar(s) != (Object) s && !(scalar(s) instanceof String))
            return "'" + s.replace("'", "''") + "'";
        return s;
    }
}
