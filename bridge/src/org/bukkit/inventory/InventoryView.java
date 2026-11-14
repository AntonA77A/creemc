package org.bukkit.inventory;

import net.kyori.adventure.text.Component;
import org.bukkit.entity.HumanEntity;
import org.bukkit.event.inventory.InventoryType;

/** Ein offenes Fenster: oberes Inventar (Kiste, GUI ...) und unteres (Spielerinventar). */
public class InventoryView {
    private final Inventory top, bottom;
    private final HumanEntity player;
    private final String title;
    private final InventoryType type;

    public InventoryView(Inventory top, Inventory bottom, HumanEntity player, String title, InventoryType type) {
        this.top = top;
        this.bottom = bottom;
        this.player = player;
        this.title = title;
        this.type = type;
    }

    public Inventory getTopInventory() { return top; }

    public Inventory getBottomInventory() { return bottom; }

    public HumanEntity getPlayer() { return player; }

    public InventoryType getType() { return type; }

    public String getTitle() { return title; }

    public String getOriginalTitle() { return title; }

    public Component title() { return Component.text(title); }

    public int countSlots() { return top.getSize() + 36; }

    /** Inventar zu einer Roh-Slotnummer des Fensters (oben zuerst). */
    public Inventory getInventory(int rawSlot) {
        if (rawSlot < 0) return null;
        return rawSlot < top.getSize() ? top : bottom;
    }

    /** Roh-Slot -> Slot im jeweiligen Inventar. */
    public int convertSlot(int rawSlot) {
        int n = top.getSize();
        if (rawSlot < n) return rawSlot;
        int i = rawSlot - n;
        return i < 27 ? i + 9 : i - 27;
    }

    public ItemStack getItem(int rawSlot) {
        Inventory inv = getInventory(rawSlot);
        return inv == null ? null : inv.getItem(convertSlot(rawSlot));
    }

    public void setItem(int rawSlot, ItemStack item) {
        Inventory inv = getInventory(rawSlot);
        if (inv != null) inv.setItem(convertSlot(rawSlot), item);
    }

    public void close() { player.closeInventory(); }
}
