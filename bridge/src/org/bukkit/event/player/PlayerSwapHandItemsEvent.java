package org.bukkit.event.player;

import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;
import org.bukkit.inventory.ItemStack;

public class PlayerSwapHandItemsEvent extends PlayerEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private ItemStack mainHand;
    private ItemStack offHand;

    public PlayerSwapHandItemsEvent(Player who, ItemStack mainHand, ItemStack offHand) {
        super(who);
        this.mainHand = mainHand;
        this.offHand = offHand;
    }

    public ItemStack getMainHandItem() { return mainHand; }
    public void setMainHandItem(ItemStack value) { this.mainHand = value; }
    public ItemStack getOffHandItem() { return offHand; }
    public void setOffHandItem(ItemStack value) { this.offHand = value; }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
