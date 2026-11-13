package net.kyori.adventure.text.format;

import java.util.List;

public final class NamedTextColor implements TextColor {
    public static final NamedTextColor BLACK = new NamedTextColor("black", 0x000000, '0');
    public static final NamedTextColor DARK_BLUE = new NamedTextColor("dark_blue", 0x0000aa, '1');
    public static final NamedTextColor DARK_GREEN = new NamedTextColor("dark_green", 0x00aa00, '2');
    public static final NamedTextColor DARK_AQUA = new NamedTextColor("dark_aqua", 0x00aaaa, '3');
    public static final NamedTextColor DARK_RED = new NamedTextColor("dark_red", 0xaa0000, '4');
    public static final NamedTextColor DARK_PURPLE = new NamedTextColor("dark_purple", 0xaa00aa, '5');
    public static final NamedTextColor GOLD = new NamedTextColor("gold", 0xffaa00, '6');
    public static final NamedTextColor GRAY = new NamedTextColor("gray", 0xaaaaaa, '7');
    public static final NamedTextColor DARK_GRAY = new NamedTextColor("dark_gray", 0x555555, '8');
    public static final NamedTextColor BLUE = new NamedTextColor("blue", 0x5555ff, '9');
    public static final NamedTextColor GREEN = new NamedTextColor("green", 0x55ff55, 'a');
    public static final NamedTextColor AQUA = new NamedTextColor("aqua", 0x55ffff, 'b');
    public static final NamedTextColor RED = new NamedTextColor("red", 0xff5555, 'c');
    public static final NamedTextColor LIGHT_PURPLE = new NamedTextColor("light_purple", 0xff55ff, 'd');
    public static final NamedTextColor YELLOW = new NamedTextColor("yellow", 0xffff55, 'e');
    public static final NamedTextColor WHITE = new NamedTextColor("white", 0xffffff, 'f');
    static final List<NamedTextColor> VALUES = List.of(BLACK, DARK_BLUE, DARK_GREEN, DARK_AQUA, DARK_RED, DARK_PURPLE,
            GOLD, GRAY, DARK_GRAY, BLUE, GREEN, AQUA, RED, LIGHT_PURPLE, YELLOW, WHITE);

    private final String name;
    private final int value;
    final char code;

    private NamedTextColor(String name, int value, char code) {
        this.name = name;
        this.value = value;
        this.code = code;
    }

    @Override
    public int value() { return value; }

    public static NamedTextColor namedColor(int value) {
        for (NamedTextColor c : VALUES) if (c.value == value) return c;
        return null;
    }

    public static NamedTextColor nearestTo(TextColor any) {
        char code = TextColor.legacyCode(any);
        for (NamedTextColor c : VALUES) if (c.code == code) return c;
        return WHITE;
    }

    public static NamedTextColor byName(String name) {
        for (NamedTextColor c : VALUES) if (c.name.equalsIgnoreCase(name)) return c;
        return null;
    }

    public char legacyChar() { return code; }

    @Override
    public String toString() { return name; }
}
