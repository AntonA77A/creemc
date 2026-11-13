package org.bukkit.event.block;

import org.bukkit.block.Block;
import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;
import org.bukkit.inventory.ItemStack;

public class BlockPlaceEvent extends BlockEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private Player player;
    private Block placedAgainst;
    private ItemStack itemInHand;
    private boolean canBuild;

    public BlockPlaceEvent(Block block, Player player, Block placedAgainst, ItemStack itemInHand, boolean canBuild) {
        super(block);
        this.player = player;
        this.placedAgainst = placedAgainst;
        this.itemInHand = itemInHand;
        this.canBuild = canBuild;
    }

    public Block getBlockPlaced() { return block; }

    public Player getPlayer() { return player; }

    public Block getBlockAgainst() { return placedAgainst; }

    public ItemStack getItemInHand() { return itemInHand; }

    public boolean canBuild() { return canBuild; }

    public void setBuild(boolean canBuild) { this.canBuild = canBuild; }

    @Override
    public boolean isCancelled() { return cancelled; }

    @Override
    public void setCancelled(boolean cancel) { cancelled = cancel; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
