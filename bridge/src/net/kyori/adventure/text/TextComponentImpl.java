package net.kyori.adventure.text;

import java.util.ArrayList;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;
import net.kyori.adventure.text.format.TextColor;
import net.kyori.adventure.text.format.TextDecoration;

final class TextComponentImpl implements TextComponent {
    final String content;
    final TextColor color;
    final Map<TextDecoration, Boolean> deco;
    final List<Component> children;

    TextComponentImpl(String content) { this(content, null, new EnumMap<>(TextDecoration.class), List.of()); }

    TextComponentImpl(String content, TextColor color, Map<TextDecoration, Boolean> deco, List<Component> children) {
        this.content = content;
        this.color = color;
        this.deco = deco;
        this.children = children;
    }

    @Override
    public String content() { return content; }

    @Override
    public TextComponent content(String c) { return new TextComponentImpl(c, color, deco, children); }

    @Override
    public TextComponent append(Component other) {
        List<Component> l = new ArrayList<>(children);
        l.add(other);
        return new TextComponentImpl(content, color, deco, l);
    }

    @Override
    public TextComponent append(ComponentLike other) { return append(other.asComponent()); }

    @Override
    public TextComponent color(TextColor c) { return new TextComponentImpl(content, c, deco, children); }

    @Override
    public TextColor color() { return color; }

    @Override
    public TextComponent decorate(TextDecoration d) { return decoration(d, true); }

    @Override
    public TextComponent decoration(TextDecoration d, boolean flag) {
        Map<TextDecoration, Boolean> m = new EnumMap<>(TextDecoration.class);
        m.putAll(deco);
        m.put(d, flag);
        return new TextComponentImpl(content, color, m, children);
    }

    @Override
    public boolean hasDecoration(TextDecoration d) { return Boolean.TRUE.equals(deco.get(d)); }

    @Override
    public List<Component> children() { return children; }

    @Override
    public String toLegacy() {
        StringBuilder b = new StringBuilder();
        legacy(b, null, new EnumMap<>(TextDecoration.class));
        return b.toString();
    }

    void legacy(StringBuilder b, TextColor parentColor, Map<TextDecoration, Boolean> parentDeco) {
        TextColor c = color != null ? color : parentColor;
        Map<TextDecoration, Boolean> d = new EnumMap<>(TextDecoration.class);
        d.putAll(parentDeco);
        d.putAll(deco);
        if (!content.isEmpty()) {
            b.append('§').append(c == null ? 'r' : TextColor.legacyCode(c));
            for (Map.Entry<TextDecoration, Boolean> e : d.entrySet())
                if (e.getValue()) b.append('§').append(e.getKey().code);
            b.append(content);
        }
        for (Component child : children) {
            if (child instanceof TextComponentImpl t) t.legacy(b, c, d);
            else b.append(child.toLegacy());
        }
    }

    @Override
    public String toPlain() {
        StringBuilder b = new StringBuilder(content);
        for (Component child : children) b.append(child.toPlain());
        return b.toString();
    }

    @Override
    public String toString() { return toPlain(); }

    @Override
    public boolean equals(Object o) { return o instanceof Component c && c.toLegacy().equals(toLegacy()); }

    @Override
    public int hashCode() { return toLegacy().hashCode(); }

    static final class BuilderImpl implements TextComponent.Builder {
        private TextComponent c = new TextComponentImpl("");

        @Override
        public Builder content(String content) { c = c.content(content); return this; }

        @Override
        public Builder color(TextColor color) { c = c.color(color); return this; }

        @Override
        public Builder decorate(TextDecoration d) { c = c.decorate(d); return this; }

        @Override
        public Builder decoration(TextDecoration d, boolean flag) { c = c.decoration(d, flag); return this; }

        @Override
        public Builder append(Component component) { c = c.append(component); return this; }

        @Override
        public Builder append(ComponentLike component) { c = c.append(component); return this; }

        @Override
        public TextComponent build() { return c; }
    }
}
