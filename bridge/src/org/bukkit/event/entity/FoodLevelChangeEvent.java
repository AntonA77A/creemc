package org.bukkit.event.entity;

import org.bukkit.entity.HumanEntity;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;

public class FoodLevelChangeEvent extends EntityEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private int level;

    public FoodLevelChangeEvent(HumanEntity who, int level) {
        super(who);
        this.level = level;
    }

    public int getFoodLevel() { return level; }
    public void setFoodLevel(int value) { this.level = value; }
    @Override public HumanEntity getEntity() { return (HumanEntity) entity; }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
