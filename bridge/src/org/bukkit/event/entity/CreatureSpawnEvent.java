package org.bukkit.event.entity;

import org.bukkit.entity.Entity;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;

public class CreatureSpawnEvent extends EntityEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private SpawnReason reason;

    public CreatureSpawnEvent(Entity who, SpawnReason reason) {
        super(who);
        this.reason = reason;
    }

    public SpawnReason getSpawnReason() { return reason; }
    public enum SpawnReason { NATURAL, SPAWNER_EGG, SPAWNER, COMMAND, CUSTOM, DEFAULT, BREEDING, DISPENSE_EGG }
    public org.bukkit.Location getLocation() { return entity.getLocation(); }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
