package net.kyori.adventure.text.serializer;

import net.kyori.adventure.text.Component;

public interface ComponentSerializer<I extends Component, O extends Component, R> {
    O deserialize(R input);

    R serialize(I component);
}
