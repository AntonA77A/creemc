package org.bukkit.event.player;

import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;

public class PlayerItemHeldEvent extends PlayerEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private int previous;
    private int current;

    public PlayerItemHeldEvent(Player who, int previous, int current) {
        super(who);
        this.previous = previous;
        this.current = current;
    }

    public int getPreviousSlot() { return previous; }
    public int getNewSlot() { return current; }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
