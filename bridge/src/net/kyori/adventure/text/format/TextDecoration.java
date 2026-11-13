package net.kyori.adventure.text.format;

public enum TextDecoration {
    OBFUSCATED('k'), BOLD('l'), STRIKETHROUGH('m'), UNDERLINED('n'), ITALIC('o');

    public final char code;

    TextDecoration(char code) { this.code = code; }

    public enum State { NOT_SET, FALSE, TRUE }
}
