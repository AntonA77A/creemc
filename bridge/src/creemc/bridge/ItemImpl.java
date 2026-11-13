package creemc.bridge;

import org.bukkit.Location;
import org.bukkit.entity.EntityType;
import org.bukkit.entity.Item;
import org.bukkit.inventory.ItemStack;

/** Rückgabe von World#dropItem (das echte Entity lebt im Python-Server). */
final class ItemImpl extends EntityImpl implements Item {
    private final ItemStack stack;

    ItemImpl(Bridge bridge, ItemStack stack, Location l) {
        super(bridge, -1);
        this.stack = stack;
        this.type = "item";
        this.x = l.getX();
        this.y = l.getY();
        this.z = l.getZ();
    }

    @Override public ItemStack getItemStack() { return stack.clone(); }

    @Override public int getPickupDelay() { return 10; }

    @Override public void setPickupDelay(int delay) { }

    @Override public EntityType getType() { return EntityType.ITEM; }
}
