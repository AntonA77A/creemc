package org.bukkit.inventory;

/** Besitzer eines Inventars (z.B. eigene GUI-Klasse eines Plugins). */
public interface InventoryHolder {
    Inventory getInventory();
}
