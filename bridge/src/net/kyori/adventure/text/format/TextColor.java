package net.kyori.adventure.text.format;

public interface TextColor {
    int value();

    default int red() { return value() >> 16 & 0xFF; }

    default int green() { return value() >> 8 & 0xFF; }

    default int blue() { return value() & 0xFF; }

    default String asHexString() { return String.format("#%06x", value()); }

    static TextColor color(int rgb) {
        int v = rgb & 0xFFFFFF;
        return () -> v;
    }

    static TextColor color(int r, int g, int b) { return color((r & 0xFF) << 16 | (g & 0xFF) << 8 | b & 0xFF); }

    static TextColor fromHexString(String hex) {
        if (hex == null || !hex.startsWith("#") || hex.length() != 7) return null;
        try {
            return color(Integer.parseInt(hex.substring(1), 16));
        } catch (NumberFormatException e) {
            return null;
        }
    }

    static TextColor fromCSSHexString(String hex) { return fromHexString(hex); }

    /** Nächster der 16 Chat-Farbcodes (§0-§f). */
    static char legacyCode(TextColor c) {
        if (c instanceof NamedTextColor n) return n.code;
        char best = 'f';
        long bestD = Long.MAX_VALUE;
        for (NamedTextColor n : NamedTextColor.VALUES) {
            long dr = n.red() - c.red(), dg = n.green() - c.green(), db = n.blue() - c.blue();
            long d = dr * dr + dg * dg + db * db;
            if (d < bestD) {
                bestD = d;
                best = n.code;
            }
        }
        return best;
    }
}
