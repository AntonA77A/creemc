package creemc.api;

import java.util.HashMap;
import java.util.Map;
import java.util.function.BiConsumer;
import java.util.function.Consumer;
import org.bukkit.Bukkit;
import org.bukkit.entity.Player;
import org.bukkit.event.EventPriority;
import org.bukkit.event.Listener;
import org.bukkit.event.inventory.ClickType;
import org.bukkit.event.inventory.InventoryClickEvent;
import org.bukkit.event.inventory.InventoryCloseEvent;
import org.bukkit.inventory.Inventory;
import org.bukkit.inventory.InventoryHolder;
import org.bukkit.inventory.ItemStack;
import org.bukkit.plugin.Plugin;

/**
 * Menü-GUI mit Klick-Aktionen pro Slot. Spieler können nichts herausnehmen.
 *
 * <pre>
 * Gui menu = gui("&amp;6Menü", 3);
 * menu.item(13, CreeMC.item("diamond").name("&amp;bHeilen"), (p, click) -&gt; CreeMC.heal(p));
 * menu.fill(CreeMC.item("gray_stained_glass_pane").name(" "));
 * menu.open(player);
 * </pre>
 */
public final class Gui implements InventoryHolder, Listener {
    private final Inventory inventory;
    private final Map<Integer, BiConsumer<Player, ClickType>> actions = new HashMap<>();
    private Consumer<Player> onClose;

    /** rows = 1 bis 6 Reihen zu je 9 Slots. */
    public Gui(Plugin plugin, String title, int rows) {
        inventory = Bukkit.createInventory(this, Math.max(1, Math.min(6, rows)) * 9, CreeMC.color(title));
        Bukkit.getPluginManager().registerEvent(InventoryClickEvent.class, this, EventPriority.LOWEST, (l, e) -> click((InventoryClickEvent) e), plugin);
        Bukkit.getPluginManager().registerEvent(InventoryCloseEvent.class, this, EventPriority.MONITOR, (l, e) -> close((InventoryCloseEvent) e), plugin);
    }

    private void click(InventoryClickEvent e) {
        if (e.getView().getTopInventory() != inventory) return;
        e.setCancelled(true);   // nichts herausnehmen oder hineinlegen
        BiConsumer<Player, ClickType> a = e.getRawSlot() < inventory.getSize() ? actions.get(e.getRawSlot()) : null;
        if (a != null && e.getWhoClicked() instanceof Player p) a.accept(p, e.getClick());
    }

    private void close(InventoryCloseEvent e) {
        if (e.getView().getTopInventory() == inventory && onClose != null && e.getPlayer() instanceof Player p) onClose.accept(p);
    }

    /** Item ohne Aktion. */
    public Gui item(int slot, ItemStack item) {
        inventory.setItem(slot, item);
        actions.remove(slot);
        return this;
    }

    public Gui item(int slot, ItemBuilder item) { return item(slot, CreeMC.stack(item, 1)); }

    /** Item mit Aktion beim Anklicken. */
    public Gui item(int slot, ItemStack item, BiConsumer<Player, ClickType> action) {
        inventory.setItem(slot, item);
        actions.put(slot, action);
        return this;
    }

    public Gui item(int slot, ItemBuilder item, BiConsumer<Player, ClickType> action) { return item(slot, CreeMC.stack(item, 1), action); }

    /** Alle leeren Slots füllen (z.B. mit Glasscheiben). */
    public Gui fill(ItemBuilder item) {
        ItemStack st = CreeMC.stack(item, 1);
        for (int i = 0; i < inventory.getSize(); i++) if (inventory.getItem(i) == null) inventory.setItem(i, st);
        return this;
    }

    public Gui clear() {
        inventory.clear();
        actions.clear();
        return this;
    }

    public Gui onClose(Consumer<Player> action) {
        onClose = action;
        return this;
    }

    public void open(Player p) { p.openInventory(inventory); }

    public int size() { return inventory.getSize(); }

    @Override public Inventory getInventory() { return inventory; }
}
