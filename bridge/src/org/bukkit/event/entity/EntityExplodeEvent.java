package org.bukkit.event.entity;

import java.util.List;
import org.bukkit.Location;
import org.bukkit.block.Block;
import org.bukkit.entity.Entity;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;

public class EntityExplodeEvent extends EntityEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private Location location;
    private List<Block> blocks;
    private float yield;

    public EntityExplodeEvent(Entity who, Location location, List<Block> blocks, float yield) {
        super(who);
        this.location = location;
        this.blocks = blocks;
        this.yield = yield;
    }

    public Location getLocation() { return location; }
    public List<Block> blockList() { return blocks; }
    public float getYield() { return yield; }
    public void setYield(float value) { this.yield = value; }
    @Override public boolean isCancelled() { return cancelled; }
    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }
    @Override public HandlerList getHandlers() { return HANDLERS; }
    public static HandlerList getHandlerList() { return HANDLERS; }
}
