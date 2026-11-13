package org.bukkit.event.entity;

import org.bukkit.entity.Entity;
import org.bukkit.event.HandlerList;
import org.bukkit.inventory.ItemStack;

public class EntityDeathEvent extends EntityEvent {
    private static final HandlerList HANDLERS = new HandlerList();
    private java.util.List<ItemStack> drops;
    private int droppedExp;

    public EntityDeathEvent(Entity what, java.util.List<ItemStack> drops, int droppedExp) {
        super(what);
        this.drops = drops;
        this.droppedExp = droppedExp;
    }

    @Override
    public org.bukkit.entity.LivingEntity getEntity() { return (org.bukkit.entity.LivingEntity) entity; }

    public java.util.List<ItemStack> getDrops() { return drops; }

    public int getDroppedExp() { return droppedExp; }

    public void setDroppedExp(int droppedExp) { this.droppedExp = droppedExp; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
