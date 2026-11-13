package org.bukkit;

import java.util.regex.Pattern;

public enum ChatColor {
    BLACK('0'), DARK_BLUE('1'), DARK_GREEN('2'), DARK_AQUA('3'), DARK_RED('4'), DARK_PURPLE('5'), GOLD('6'),
    GRAY('7'), DARK_GRAY('8'), BLUE('9'), GREEN('a'), AQUA('b'), RED('c'), LIGHT_PURPLE('d'), YELLOW('e'),
    WHITE('f'), MAGIC('k', true), BOLD('l', true), STRIKETHROUGH('m', true), UNDERLINE('n', true),
    ITALIC('o', true), RESET('r');

    public static final char COLOR_CHAR = '§';
    private static final Pattern STRIP = Pattern.compile("(?i)" + COLOR_CHAR + "[0-9A-FK-ORX]");
    private final char code;
    private final boolean format;

    ChatColor(char code) { this(code, false); }

    ChatColor(char code, boolean format) {
        this.code = code;
        this.format = format;
    }

    public char getChar() { return code; }

    public boolean isFormat() { return format; }

    public boolean isColor() { return !format && this != RESET; }

    @Override
    public String toString() { return new String(new char[]{COLOR_CHAR, code}); }

    public static ChatColor getByChar(char c) {
        for (ChatColor col : values()) if (col.code == Character.toLowerCase(c)) return col;
        return null;
    }

    public static ChatColor getByChar(String s) { return s == null || s.isEmpty() ? null : getByChar(s.charAt(0)); }

    public static String stripColor(String input) { return input == null ? null : STRIP.matcher(input).replaceAll(""); }

    public static String translateAlternateColorCodes(char alt, String text) {
        char[] b = text.toCharArray();
        for (int i = 0; i < b.length - 1; i++) {
            if (b[i] == alt && "0123456789AaBbCcDdEeFfKkLlMmNnOoRrXx".indexOf(b[i + 1]) > -1) {
                b[i] = COLOR_CHAR;
                b[i + 1] = Character.toLowerCase(b[i + 1]);
            }
        }
        return new String(b);
    }

    public static String getLastColors(String input) {
        String result = "";
        for (int i = input.length() - 2; i >= 0; i--) {
            if (input.charAt(i) == COLOR_CHAR) {
                ChatColor c = getByChar(input.charAt(i + 1));
                if (c != null) {
                    result = c + result;
                    if (c.isColor() || c == RESET) break;
                }
            }
        }
        return result;
    }
}
