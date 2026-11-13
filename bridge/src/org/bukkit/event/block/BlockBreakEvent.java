package org.bukkit.event.block;

import org.bukkit.block.Block;
import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;

public class BlockBreakEvent extends BlockEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private Player player;
    private boolean dropItems;
    private int expToDrop;

    public BlockBreakEvent(Block block, Player player, boolean dropItems, int expToDrop) {
        super(block);
        this.player = player;
        this.dropItems = dropItems;
        this.expToDrop = expToDrop;
    }

    public Player getPlayer() { return player; }

    public boolean isDropItems() { return dropItems; }

    public void setDropItems(boolean dropItems) { this.dropItems = dropItems; }

    public int getExpToDrop() { return expToDrop; }

    public void setExpToDrop(int expToDrop) { this.expToDrop = expToDrop; }

    @Override
    public boolean isCancelled() { return cancelled; }

    @Override
    public void setCancelled(boolean cancel) { cancelled = cancel; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
