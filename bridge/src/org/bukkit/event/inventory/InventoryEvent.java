package org.bukkit.event.inventory;

import java.util.List;
import org.bukkit.entity.HumanEntity;
import org.bukkit.event.Event;
import org.bukkit.inventory.Inventory;
import org.bukkit.inventory.InventoryView;

public abstract class InventoryEvent extends Event {
    protected InventoryView transaction;

    public InventoryEvent(InventoryView transaction) { this.transaction = transaction; }

    public Inventory getInventory() { return transaction.getTopInventory(); }

    public InventoryView getView() { return transaction; }

    public List<HumanEntity> getViewers() { return transaction.getTopInventory().getViewers(); }
}
