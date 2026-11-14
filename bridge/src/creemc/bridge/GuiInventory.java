package creemc.bridge;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import org.bukkit.Material;
import org.bukkit.entity.HumanEntity;
import org.bukkit.event.inventory.InventoryType;
import org.bukkit.inventory.Inventory;
import org.bukkit.inventory.InventoryHolder;
import org.bukkit.inventory.ItemStack;

/**
 * Inventar aus Bukkit#createInventory (GUIs, eigene Kisten). Der Inhalt lebt hier; offene Fenster im Server
 * werden bei jeder Änderung aktualisiert. id = -1: nur eine Momentaufnahme (z.B. Vanilla-Kiste in einem Event).
 */
public final class GuiInventory implements Inventory {
    private static final AtomicInteger IDS = new AtomicInteger();
    static final Map<Integer, GuiInventory> ALL = new ConcurrentHashMap<>();

    final int id;
    private final ItemStack[] items;
    private final InventoryHolder holder;
    private final InventoryType type;
    final String title;
    final Set<HumanEntity> viewers = new LinkedHashSet<>();

    public GuiInventory(InventoryHolder holder, int size, String title, InventoryType type, boolean live) {
        if (live && (size <= 0 || size > 54 || (type == InventoryType.CHEST && size % 9 != 0)))
            throw new IllegalArgumentException("Größe muss ein Vielfaches von 9 zwischen 9 und 54 sein: " + size);
        this.id = live ? IDS.incrementAndGet() : -1;
        this.items = new ItemStack[size];
        this.holder = holder;
        this.title = title == null ? type.getDefaultTitle() : title;
        this.type = type;
        if (live) ALL.put(id, this);
    }

    static GuiInventory byId(int id) { return ALL.get(id); }

    /** Vanilla-Fenstertyp ("menu"-Registry) für den Server. */
    int menu() {
        return switch (type) {
            case HOPPER -> 16;
            case DISPENSER, DROPPER -> 6;
            default -> Math.max(0, Math.min(5, items.length / 9 - 1));
        };
    }

    List<Object> creeItems() {
        List<Object> out = new ArrayList<>();
        for (ItemStack s : items) out.add(s == null || s.isEmpty() ? null : List.of(s.toCreeItem(), s.getAmount()));
        return out;
    }

    /** Vom Server nach einem Klick: neuer Inhalt (ohne zurückzuschicken). */
    void updateFromServer(List<Object> list) {
        for (int i = 0; i < items.length; i++) {
            List<Object> st = i < list.size() ? Json.list(list.get(i)) : List.of();
            items[i] = st.size() >= 2 ? ItemStack.fromCree(Json.str(st.get(0), null), (int) Json.num(st.get(1), 1)) : null;
        }
    }

    private void push(int slot) {
        if (id < 0 || Bridge.get() == null) return;
        ItemStack s = items[slot];
        Bridge.get().call("gui_set", "id", id, "slot", slot, "item", s == null || s.isEmpty() ? null : s.toCreeItem(),
                "count", s == null ? 0 : s.getAmount());
    }

    private void pushAll() {
        if (id >= 0 && Bridge.get() != null) Bridge.get().call("gui_contents", "id", id, "items", creeItems());
    }

    @Override public int getSize() { return items.length; }

    @Override public ItemStack getItem(int index) { return index >= 0 && index < items.length ? items[index] : null; }

    @Override
    public void setItem(int index, ItemStack item) {
        if (index < 0 || index >= items.length) throw new ArrayIndexOutOfBoundsException(index);
        items[index] = item == null || item.isEmpty() ? null : item.clone();
        push(index);
    }

    @Override
    public HashMap<Integer, ItemStack> addItem(ItemStack... add) {
        HashMap<Integer, ItemStack> rest = new HashMap<>();
        for (int k = 0; k < add.length; k++) {
            ItemStack it = add[k];
            if (it == null || it.isEmpty()) continue;
            int left = it.getAmount();
            int max = it.getMaxStackSize();
            for (int pass = 0; pass < 2 && left > 0; pass++)
                for (int i = 0; i < items.length && left > 0; i++) {
                    ItemStack s = items[i];
                    if (pass == 0 && s != null && s.isSimilar(it) && s.getAmount() < max) {
                        int move = Math.min(left, max - s.getAmount());
                        s.setAmount(s.getAmount() + move);
                        left -= move;
                    } else if (pass == 1 && s == null) {
                        ItemStack n = it.clone();
                        n.setAmount(Math.min(left, max));
                        items[i] = n;
                        left -= n.getAmount();
                    }
                }
            if (left > 0) {
                ItemStack r = it.clone();
                r.setAmount(left);
                rest.put(k, r);
            }
        }
        pushAll();
        return rest;
    }

    @Override
    public HashMap<Integer, ItemStack> removeItem(ItemStack... rem) {
        HashMap<Integer, ItemStack> rest = new HashMap<>();
        for (int k = 0; k < rem.length; k++) {
            ItemStack it = rem[k];
            if (it == null) continue;
            int left = it.getAmount();
            for (int i = 0; i < items.length && left > 0; i++) {
                ItemStack s = items[i];
                if (s == null || !s.isSimilar(it)) continue;
                int take = Math.min(left, s.getAmount());
                left -= take;
                if (s.getAmount() - take <= 0) items[i] = null;
                else s.setAmount(s.getAmount() - take);
            }
            if (left > 0) {
                ItemStack r = it.clone();
                r.setAmount(left);
                rest.put(k, r);
            }
        }
        pushAll();
        return rest;
    }

    @Override public ItemStack[] getContents() { return items.clone(); }

    @Override
    public void setContents(ItemStack[] c) {
        for (int i = 0; i < items.length; i++) items[i] = i < c.length && c[i] != null && !c[i].isEmpty() ? c[i].clone() : null;
        pushAll();
    }

    @Override public boolean contains(Material material) { return contains(material, 1); }

    @Override
    public boolean contains(Material material, int amount) {
        int n = 0;
        for (ItemStack s : items) if (s != null && s.getType() == material) n += s.getAmount();
        return n >= amount;
    }

    @Override
    public boolean containsAtLeast(ItemStack item, int amount) {
        int n = 0;
        for (ItemStack s : items) if (s != null && s.isSimilar(item)) n += s.getAmount();
        return n >= amount;
    }

    @Override
    public int firstEmpty() {
        for (int i = 0; i < items.length; i++) if (items[i] == null) return i;
        return -1;
    }

    @Override
    public boolean isEmpty() {
        for (ItemStack s : items) if (s != null) return false;
        return true;
    }

    @Override
    public void clear() {
        java.util.Arrays.fill(items, null);
        pushAll();
    }

    @Override public void clear(int index) { setItem(index, null); }

    @Override
    public void remove(Material material) {
        for (int i = 0; i < items.length; i++) if (items[i] != null && items[i].getType() == material) items[i] = null;
        pushAll();
    }

    @Override public InventoryHolder getHolder() { return holder; }

    @Override public List<HumanEntity> getViewers() { return new ArrayList<>(viewers); }

    @Override public InventoryType getType() { return type; }

    @Override
    public Iterator<ItemStack> iterator() { return java.util.Arrays.asList(items.clone()).iterator(); }
}
