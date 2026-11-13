package org.bukkit.inventory;

public interface PlayerInventory extends Inventory {
    ItemStack getItemInMainHand();

    void setItemInMainHand(ItemStack item);

    ItemStack getItemInOffHand();

    void setItemInOffHand(ItemStack item);

    int getHeldItemSlot();

    void setHeldItemSlot(int slot);

    ItemStack getHelmet();

    ItemStack getChestplate();

    ItemStack getLeggings();

    ItemStack getBoots();

    void setHelmet(ItemStack item);

    void setChestplate(ItemStack item);

    void setLeggings(ItemStack item);

    void setBoots(ItemStack item);

    ItemStack[] getArmorContents();
}
