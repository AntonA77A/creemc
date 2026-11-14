package org.bukkit.event.player;

import org.bukkit.entity.Item;
import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;

public class PlayerDropItemEvent extends PlayerEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private Item drop;

    public PlayerDropItemEvent(Player who, Item drop) {
        super(who);
        this.drop = drop;
    }

    public Item getItemDrop() { return drop; }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
