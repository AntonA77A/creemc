package net.kyori.adventure.text.minimessage;

import java.util.ArrayDeque;
import java.util.Deque;
import net.kyori.adventure.text.Component;
import net.kyori.adventure.text.TextComponent;
import net.kyori.adventure.text.format.NamedTextColor;
import net.kyori.adventure.text.format.TextColor;
import net.kyori.adventure.text.format.TextDecoration;
import net.kyori.adventure.text.serializer.ComponentSerializer;

/** MiniMessage-Teilmenge: Farben (&lt;red&gt;, &lt;#ff0000&gt;, &lt;color:..&gt;), Formatierungen, &lt;reset&gt;, &lt;newline&gt;. */
public interface MiniMessage extends ComponentSerializer<Component, Component, String> {
    static MiniMessage miniMessage() { return Impl.INSTANCE; }

    @Override
    Component deserialize(String input);

    @Override
    String serialize(Component component);

    default String stripTags(String input) { return deserialize(input).toPlain(); }

    default String escapeTags(String input) { return input.replace("<", "\\<"); }

    final class Impl implements MiniMessage {
    static final Impl INSTANCE = new Impl();

    private record Style(TextColor color, TextDecoration deco) { }

    @Override
    public Component deserialize(String input) {
        TextComponent root = Component.empty();
        Deque<Style> stack = new ArrayDeque<>();
        StringBuilder text = new StringBuilder();
        int i = 0;
        while (i < input.length()) {
            char c = input.charAt(i);
            if (c == '\\' && i + 1 < input.length() && input.charAt(i + 1) == '<') {
                text.append('<');
                i += 2;
                continue;
            }
            int end = c == '<' ? input.indexOf('>', i) : -1;
            if (end < 0) {
                text.append(c);
                i++;
                continue;
            }
            String tag = input.substring(i + 1, end).toLowerCase();
            root = flush(root, text, stack);
            boolean close = tag.startsWith("/");
            String name = close ? tag.substring(1) : tag;
            if (name.startsWith("color:") || name.startsWith("c:") || name.startsWith("colour:"))
                name = name.substring(name.indexOf(':') + 1);
            if (name.equals("reset")) stack.clear();
            else if (name.equals("newline") || name.equals("br")) text.append('\n');
            else if (close) {
                if (!stack.isEmpty()) stack.pop();
            } else {
                TextColor col = name.startsWith("#") ? TextColor.fromHexString(name) : NamedTextColor.byName(name.replace("grey", "gray"));
                TextDecoration deco = switch (name) {
                    case "b", "bold" -> TextDecoration.BOLD;
                    case "i", "em", "italic" -> TextDecoration.ITALIC;
                    case "u", "underlined" -> TextDecoration.UNDERLINED;
                    case "st", "strikethrough" -> TextDecoration.STRIKETHROUGH;
                    case "obf", "obfuscated" -> TextDecoration.OBFUSCATED;
                    default -> null;
                };
                if (col != null || deco != null) stack.push(new Style(col, deco));
                else text.append(input, i, end + 1);   // unbekanntes Tag: als Text lassen
            }
            i = end + 1;
        }
        return flush(root, text, stack);
    }

    private static TextComponent flush(TextComponent root, StringBuilder text, Deque<Style> stack) {
        if (text.length() == 0) return root;
        TextComponent part = Component.text(text.toString());
        TextColor col = null;
        for (Style s : stack) {   // oben auf dem Stapel = zuletzt geöffnet
            if (col == null && s.color() != null) col = s.color();
            if (s.deco() != null) part = part.decorate(s.deco());
        }
        if (col != null) part = part.color(col);
        text.setLength(0);
        return root.append(part);
    }

    @Override
    public String serialize(Component component) { return component.toPlain(); }
    }
}
