package org.bukkit.inventory;

import java.util.HashMap;
import org.bukkit.Material;

public interface Inventory extends Iterable<ItemStack> {
    int getSize();

    ItemStack getItem(int index);

    void setItem(int index, ItemStack item);

    HashMap<Integer, ItemStack> addItem(ItemStack... items);

    HashMap<Integer, ItemStack> removeItem(ItemStack... items);

    ItemStack[] getContents();

    void setContents(ItemStack[] items);

    boolean contains(Material material);

    boolean contains(Material material, int amount);

    boolean containsAtLeast(ItemStack item, int amount);

    int firstEmpty();

    boolean isEmpty();

    void clear();

    void clear(int index);

    void remove(Material material);

    default void remove(ItemStack item) {
        for (int i = 0; i < getSize(); i++) {
            ItemStack s = getItem(i);
            if (s != null && s.equals(item)) clear(i);
        }
    }

    default InventoryHolder getHolder() { return null; }

    default java.util.List<org.bukkit.entity.HumanEntity> getViewers() { return java.util.List.of(); }

    default org.bukkit.event.inventory.InventoryType getType() { return org.bukkit.event.inventory.InventoryType.PLAYER; }

    default int first(Material material) {
        for (int i = 0; i < getSize(); i++) {
            ItemStack s = getItem(i);
            if (s != null && s.getType() == material) return i;
        }
        return -1;
    }

    default int first(ItemStack item) {
        for (int i = 0; i < getSize(); i++) {
            ItemStack s = getItem(i);
            if (s != null && s.equals(item)) return i;
        }
        return -1;
    }

    default boolean contains(ItemStack item) { return first(item) >= 0; }

    default java.util.HashMap<Integer, ? extends ItemStack> all(Material material) {
        java.util.HashMap<Integer, ItemStack> out = new java.util.HashMap<>();
        for (int i = 0; i < getSize(); i++) {
            ItemStack s = getItem(i);
            if (s != null && s.getType() == material) out.put(i, s);
        }
        return out;
    }

    default int getMaxStackSize() { return 64; }

    default ItemStack[] getStorageContents() { return getContents(); }

    default void setStorageContents(ItemStack[] items) { setContents(items); }

    default org.bukkit.Location getLocation() { return null; }

    default void close() {
        for (org.bukkit.entity.HumanEntity h : new java.util.ArrayList<>(getViewers())) h.closeInventory();
    }
}
