package org.bukkit.event.inventory;

import org.bukkit.entity.HumanEntity;
import org.bukkit.event.HandlerList;
import org.bukkit.inventory.InventoryView;

public class InventoryCloseEvent extends InventoryEvent {
    private static final HandlerList HANDLERS = new HandlerList();

    public enum Reason { UNKNOWN, TELEPORT, CANT_USE, UNLOADED, OPEN_NEW, PLAYER, DISCONNECT, DEATH, PLUGIN }

    private final Reason reason;

    public InventoryCloseEvent(InventoryView view) { this(view, Reason.PLAYER); }

    public InventoryCloseEvent(InventoryView view, Reason reason) {
        super(view);
        this.reason = reason;
    }

    public HumanEntity getPlayer() { return transaction.getPlayer(); }

    public Reason getReason() { return reason; }

    @Override public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
