package net.kyori.adventure.text.serializer.legacy;

import net.kyori.adventure.text.Component;
import net.kyori.adventure.text.TextComponent;
import net.kyori.adventure.text.format.NamedTextColor;
import net.kyori.adventure.text.format.TextDecoration;
import net.kyori.adventure.text.serializer.ComponentSerializer;

public interface LegacyComponentSerializer extends ComponentSerializer<Component, TextComponent, String> {
    char SECTION_CHAR = '§';
    char AMPERSAND_CHAR = '&';

    static LegacyComponentSerializer legacySection() { return legacy(SECTION_CHAR); }

    static LegacyComponentSerializer legacyAmpersand() { return legacy(AMPERSAND_CHAR); }

    static LegacyComponentSerializer legacy(char ch) { return new Impl(ch); }

    @Override
    TextComponent deserialize(String input);

    @Override
    String serialize(Component component);

    final class Impl implements LegacyComponentSerializer {
        private static final NamedTextColor[] COLORS = {NamedTextColor.BLACK, NamedTextColor.DARK_BLUE, NamedTextColor.DARK_GREEN,
                NamedTextColor.DARK_AQUA, NamedTextColor.DARK_RED, NamedTextColor.DARK_PURPLE, NamedTextColor.GOLD,
                NamedTextColor.GRAY, NamedTextColor.DARK_GRAY, NamedTextColor.BLUE, NamedTextColor.GREEN,
                NamedTextColor.AQUA, NamedTextColor.RED, NamedTextColor.LIGHT_PURPLE, NamedTextColor.YELLOW, NamedTextColor.WHITE};
        private final char ch;

        Impl(char ch) { this.ch = ch; }

        @Override
        public TextComponent deserialize(String input) {
            TextComponent root = Component.empty();
            TextComponent cur = Component.text("");
            StringBuilder text = new StringBuilder();
            for (int i = 0; i < input.length(); i++) {
                char c = input.charAt(i);
                if (c == ch && i + 1 < input.length()) {
                    char code = Character.toLowerCase(input.charAt(++i));
                    if (text.length() > 0) {
                        root = root.append(cur.content(text.toString()));
                        text.setLength(0);
                    }
                    NamedTextColor col = null;
                    for (NamedTextColor n : COLORS) if (n.legacyChar() == code) col = n;
                    if (col != null) cur = Component.text("", col);
                    else if (code == 'r') cur = Component.text("");
                    else for (TextDecoration d : TextDecoration.values()) if (d.code == code) cur = cur.decorate(d);
                } else text.append(c);
            }
            if (text.length() > 0) root = root.append(cur.content(text.toString()));
            return root;
        }

        @Override
        public String serialize(Component component) {
            String s = component.toLegacy();
            return ch == SECTION_CHAR ? s : s.replace(SECTION_CHAR, ch);
        }
    }
}
