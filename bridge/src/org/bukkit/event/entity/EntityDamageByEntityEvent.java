package org.bukkit.event.entity;

import org.bukkit.entity.Entity;
import org.bukkit.event.HandlerList;

public class EntityDamageByEntityEvent extends EntityDamageEvent {
    private static final HandlerList HANDLERS = new HandlerList();
    private final Entity damager;

    public EntityDamageByEntityEvent(Entity damager, Entity damagee, DamageCause cause, double damage) {
        super(damagee, cause, damage);
        this.damager = damager;
    }

    public Entity getDamager() { return damager; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
