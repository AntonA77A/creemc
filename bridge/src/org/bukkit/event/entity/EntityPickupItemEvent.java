package org.bukkit.event.entity;

import org.bukkit.entity.Item;
import org.bukkit.entity.LivingEntity;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;

public class EntityPickupItemEvent extends EntityEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private Item item;
    private int remaining;

    public EntityPickupItemEvent(LivingEntity who, Item item, int remaining) {
        super(who);
        this.item = item;
        this.remaining = remaining;
    }

    public Item getItem() { return item; }
    public int getRemaining() { return remaining; }
    @Override public LivingEntity getEntity() { return (LivingEntity) entity; }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
