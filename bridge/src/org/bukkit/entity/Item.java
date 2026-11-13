package org.bukkit.entity;

import org.bukkit.inventory.ItemStack;

public interface Item extends Entity {
    ItemStack getItemStack();

    int getPickupDelay();

    void setPickupDelay(int delay);
}
