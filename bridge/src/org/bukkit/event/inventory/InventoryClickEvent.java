package org.bukkit.event.inventory;

import org.bukkit.entity.HumanEntity;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;
import org.bukkit.inventory.Inventory;
import org.bukkit.inventory.InventoryView;
import org.bukkit.inventory.ItemStack;

/** Klick in ein Fenster. Abbrechen = der Klick passiert nicht (typisch für GUIs). */
public class InventoryClickEvent extends InventoryEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private final int rawSlot, hotbarButton;
    private final ClickType click;
    private final InventoryAction action;
    private final InventoryType.SlotType slotType;
    private ItemStack current, cursor;
    private boolean cancelled;

    public InventoryClickEvent(InventoryView view, InventoryType.SlotType type, int rawSlot, ClickType click, InventoryAction action) {
        this(view, type, rawSlot, click, action, -1);
    }

    public InventoryClickEvent(InventoryView view, InventoryType.SlotType type, int rawSlot, ClickType click, InventoryAction action,
                               int hotbarKey) {
        super(view);
        this.slotType = type;
        this.rawSlot = rawSlot;
        this.click = click;
        this.action = action;
        this.hotbarButton = hotbarKey;
    }

    public HumanEntity getWhoClicked() { return transaction.getPlayer(); }

    public int getRawSlot() { return rawSlot; }

    public int getSlot() { return rawSlot < 0 ? rawSlot : transaction.convertSlot(rawSlot); }

    public Inventory getClickedInventory() { return transaction.getInventory(rawSlot); }

    public InventoryType.SlotType getSlotType() { return slotType; }

    public ClickType getClick() { return click; }

    public InventoryAction getAction() { return action; }

    public int getHotbarButton() { return hotbarButton; }

    public boolean isLeftClick() { return click.isLeftClick(); }

    public boolean isRightClick() { return click.isRightClick(); }

    public boolean isShiftClick() { return click.isShiftClick(); }

    public ItemStack getCurrentItem() { return current; }

    public void setCurrentItem(ItemStack item) {
        current = item;
        if (rawSlot >= 0) transaction.setItem(rawSlot, item);
    }

    public ItemStack getCursor() { return cursor; }

    /** CreeMC: vom Server gesetzt. */
    public void setCreeItems(ItemStack current, ItemStack cursor) {
        this.current = current;
        this.cursor = cursor;
    }

    public Result getResult() { return cancelled ? Result.DENY : Result.DEFAULT; }

    public void setResult(Result r) { cancelled = r == Result.DENY; }

    @Override public boolean isCancelled() { return cancelled; }

    @Override public void setCancelled(boolean cancel) { cancelled = cancel; }

    @Override public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
