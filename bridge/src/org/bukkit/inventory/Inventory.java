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
}
