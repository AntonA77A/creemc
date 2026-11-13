package org.bukkit.command;

import net.kyori.adventure.audience.Audience;
import org.bukkit.Server;
import org.bukkit.permissions.Permissible;

public interface CommandSender extends Audience, Permissible {
    void sendMessage(String message);

    void sendMessage(String... messages);

    Server getServer();

    String getName();

    default net.kyori.adventure.text.Component name() { return net.kyori.adventure.text.Component.text(getName()); }

    @Override
    default void sendLegacy(String legacyText) { sendMessage(legacyText); }
}
