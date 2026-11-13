package net.kyori.adventure.text;

import net.kyori.adventure.text.format.TextColor;
import net.kyori.adventure.text.format.TextDecoration;

public interface TextComponent extends Component {
    String content();

    TextComponent content(String content);

    @Override
    TextComponent append(Component other);

    @Override
    TextComponent append(ComponentLike other);

    @Override
    TextComponent color(TextColor color);

    @Override
    TextComponent decorate(TextDecoration decoration);

    @Override
    TextComponent decoration(TextDecoration decoration, boolean flag);

    interface Builder extends ComponentLike {
        Builder content(String content);

        Builder color(TextColor color);

        Builder decorate(TextDecoration decoration);

        Builder decoration(TextDecoration decoration, boolean flag);

        Builder append(Component component);

        Builder append(ComponentLike component);

        TextComponent build();

        @Override
        default Component asComponent() { return build(); }
    }
}
