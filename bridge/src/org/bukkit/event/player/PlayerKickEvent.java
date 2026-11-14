package org.bukkit.event.player;

import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;

public class PlayerKickEvent extends PlayerEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private String reason;
    private String leaveMessage;

    public PlayerKickEvent(Player who, String reason, String leaveMessage) {
        super(who);
        this.reason = reason;
        this.leaveMessage = leaveMessage;
    }

    public String getReason() { return reason; }
    public void setReason(String value) { this.reason = value; }
    public String getLeaveMessage() { return leaveMessage; }
    public void setLeaveMessage(String value) { this.leaveMessage = value; }
    public net.kyori.adventure.text.Component reason() { return net.kyori.adventure.text.Component.text(reason); }
    public void reason(net.kyori.adventure.text.Component c) { reason = c == null ? "" : c.toLegacy(); }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
