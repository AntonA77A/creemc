package net.kyori.adventure.text.serializer.plain;

import net.kyori.adventure.text.Component;
import net.kyori.adventure.text.TextComponent;
import net.kyori.adventure.text.serializer.ComponentSerializer;

public interface PlainTextComponentSerializer extends ComponentSerializer<Component, TextComponent, String> {
    static PlainTextComponentSerializer plainText() {
        return new PlainTextComponentSerializer() {
            @Override
            public TextComponent deserialize(String input) { return Component.text(input); }

            @Override
            public String serialize(Component component) { return component.toPlain(); }
        };
    }

    @Override
    TextComponent deserialize(String input);

    @Override
    String serialize(Component component);
}
