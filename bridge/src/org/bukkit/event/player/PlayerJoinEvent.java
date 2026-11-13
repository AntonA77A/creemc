package org.bukkit.event.player;

import org.bukkit.entity.Player;
import org.bukkit.event.HandlerList;

public class PlayerJoinEvent extends PlayerEvent {
    private static final HandlerList HANDLERS = new HandlerList();
    private String joinMessage;

    public PlayerJoinEvent(Player who, String joinMessage) {
        super(who);
        this.joinMessage = joinMessage;
    }

    public net.kyori.adventure.text.Component joinMessage() {
        return joinMessage == null ? null : net.kyori.adventure.text.Component.text(joinMessage);
    }

    public void joinMessage(net.kyori.adventure.text.Component message) { joinMessage = message == null ? null : message.toLegacy(); }

    public String getJoinMessage() { return joinMessage; }

    public void setJoinMessage(String joinMessage) { this.joinMessage = joinMessage; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
