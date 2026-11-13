package net.kyori.adventure.audience;

import net.kyori.adventure.text.Component;
import net.kyori.adventure.text.ComponentLike;

public interface Audience {
    /** CreeMC: Grundmethode, an die alle Varianten weitergeben. */
    void sendLegacy(String legacyText);

    default void sendMessage(Component message) { sendLegacy(message.toLegacy()); }

    default void sendMessage(ComponentLike message) { sendMessage(message.asComponent()); }

    default void sendActionBar(Component message) { sendLegacy(message.toLegacy()); }

    default void sendActionBar(ComponentLike message) { sendActionBar(message.asComponent()); }
}
