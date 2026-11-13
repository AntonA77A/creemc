package org.bukkit.event.player;

import org.bukkit.block.Block;
import org.bukkit.block.BlockFace;
import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;
import org.bukkit.event.block.Action;
import org.bukkit.inventory.EquipmentSlot;
import org.bukkit.inventory.ItemStack;

public class PlayerInteractEvent extends PlayerEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private Action action;
    private ItemStack item;
    private Block clickedBlock;
    private BlockFace blockFace;
    private EquipmentSlot hand;

    public PlayerInteractEvent(Player who, Action action, ItemStack item, Block clickedBlock, BlockFace blockFace, EquipmentSlot hand) {
        super(who);
        this.action = action;
        this.item = item;
        this.clickedBlock = clickedBlock;
        this.blockFace = blockFace;
        this.hand = hand;
    }

    public boolean hasItem() { return item != null && !item.isEmpty(); }

    public boolean hasBlock() { return clickedBlock != null; }

    public org.bukkit.Material getMaterial() { return hasItem() ? item.getType() : org.bukkit.Material.AIR; }

    public Action getAction() { return action; }

    public ItemStack getItem() { return item; }

    public Block getClickedBlock() { return clickedBlock; }

    public BlockFace getBlockFace() { return blockFace; }

    public EquipmentSlot getHand() { return hand; }

    @Override
    public boolean isCancelled() { return cancelled; }

    @Override
    public void setCancelled(boolean cancel) { cancelled = cancel; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
