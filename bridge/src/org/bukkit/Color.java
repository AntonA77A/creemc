package org.bukkit;

public final class Color {
    public static final Color WHITE = fromRGB(0xFFFFFF), SILVER = fromRGB(0xC0C0C0), GRAY = fromRGB(0x808080), BLACK = fromRGB(0),
            RED = fromRGB(0xFF0000), MAROON = fromRGB(0x800000), YELLOW = fromRGB(0xFFFF00), OLIVE = fromRGB(0x808000),
            LIME = fromRGB(0x00FF00), GREEN = fromRGB(0x008000), AQUA = fromRGB(0x00FFFF), TEAL = fromRGB(0x008080),
            BLUE = fromRGB(0x0000FF), NAVY = fromRGB(0x000080), FUCHSIA = fromRGB(0xFF00FF), PURPLE = fromRGB(0x800080),
            ORANGE = fromRGB(0xFFA500);

    private final int rgb;

    private Color(int rgb) { this.rgb = rgb & 0xFFFFFF; }

    public static Color fromRGB(int rgb) { return new Color(rgb); }

    public static Color fromRGB(int r, int g, int b) { return new Color((r & 255) << 16 | (g & 255) << 8 | (b & 255)); }

    public int asRGB() { return rgb; }

    public int getRed() { return rgb >> 16 & 255; }

    public int getGreen() { return rgb >> 8 & 255; }

    public int getBlue() { return rgb & 255; }

    @Override public boolean equals(Object o) { return o instanceof Color c && c.rgb == rgb; }

    @Override public int hashCode() { return rgb; }
}