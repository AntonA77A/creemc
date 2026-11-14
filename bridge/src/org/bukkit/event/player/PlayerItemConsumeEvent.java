package org.bukkit.event.player;

import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;
import org.bukkit.inventory.ItemStack;

public class PlayerItemConsumeEvent extends PlayerEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private ItemStack item;

    public PlayerItemConsumeEvent(Player who, ItemStack item) {
        super(who);
        this.item = item;
    }

    public ItemStack getItem() { return item; }
    public void setItem(ItemStack value) { this.item = value; }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
