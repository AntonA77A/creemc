package org.bukkit.event.player;

import org.bukkit.entity.Player;
import org.bukkit.event.HandlerList;

public class PlayerQuitEvent extends PlayerEvent {
    private static final HandlerList HANDLERS = new HandlerList();
    private String quitMessage;

    public PlayerQuitEvent(Player who, String quitMessage) {
        super(who);
        this.quitMessage = quitMessage;
    }

    public net.kyori.adventure.text.Component quitMessage() {
        return quitMessage == null ? null : net.kyori.adventure.text.Component.text(quitMessage);
    }

    public void quitMessage(net.kyori.adventure.text.Component message) { quitMessage = message == null ? null : message.toLegacy(); }

    public String getQuitMessage() { return quitMessage; }

    public void setQuitMessage(String quitMessage) { this.quitMessage = quitMessage; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
