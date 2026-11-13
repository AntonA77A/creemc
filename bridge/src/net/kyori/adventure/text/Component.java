package net.kyori.adventure.text;

import java.util.List;
import net.kyori.adventure.text.format.TextColor;
import net.kyori.adventure.text.format.TextDecoration;

/** Vereinfachte Adventure-Komponente: wird beim Senden in §-Farbcodes umgewandelt. */
public interface Component extends ComponentLike {
    static TextComponent empty() { return new TextComponentImpl(""); }

    static TextComponent newline() { return new TextComponentImpl("\n"); }

    static TextComponent space() { return new TextComponentImpl(" "); }

    static TextComponent text(String content) { return new TextComponentImpl(content == null ? "" : content); }

    static TextComponent text(Object value) { return text(String.valueOf(value)); }

    static TextComponent text(int value) { return text(String.valueOf(value)); }

    static TextComponent text(long value) { return text(String.valueOf(value)); }

    static TextComponent text(double value) { return text(String.valueOf(value)); }

    static TextComponent text(boolean value) { return text(String.valueOf(value)); }

    static TextComponent text(String content, TextColor color) { return text(content).color(color); }

    static TextComponent text(String content, TextColor color, TextDecoration... decorations) {
        TextComponent c = text(content, color);
        for (TextDecoration d : decorations) c = c.decorate(d);
        return c;
    }

    static TextComponent.Builder text() { return new TextComponentImpl.BuilderImpl(); }

    static TextComponent translatable(String key) { return text(key); }

    static TextComponent join(Component separator, Iterable<? extends ComponentLike> parts) {
        TextComponent out = empty();
        boolean first = true;
        for (ComponentLike p : parts) {
            if (!first) out = out.append(separator);
            first = false;
            out = out.append(p.asComponent());
        }
        return out;
    }

    Component append(Component other);

    Component append(ComponentLike other);

    Component color(TextColor color);

    TextColor color();

    Component decorate(TextDecoration decoration);

    Component decoration(TextDecoration decoration, boolean flag);

    boolean hasDecoration(TextDecoration decoration);

    List<Component> children();

    @Override
    default Component asComponent() { return this; }

    /** CreeMC: als Text mit §-Farbcodes. */
    String toLegacy();

    /** CreeMC: nur der Text. */
    String toPlain();
}
