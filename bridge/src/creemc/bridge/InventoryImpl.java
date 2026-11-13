package creemc.bridge;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import org.bukkit.Material;
import org.bukkit.inventory.ItemStack;
import org.bukkit.inventory.PlayerInventory;

/** Spielerinventar (Bukkit-Slots 0-8 Hotbar, 9-35 Inventar, 36-39 Rüstung, 40 Nebenhand). */
final class InventoryImpl implements PlayerInventory {
    private final Bridge bridge;
    private final PlayerImpl player;

    InventoryImpl(Bridge bridge, PlayerImpl player) {
        this.bridge = bridge;
        this.player = player;
    }

    /** Bukkit-Slot -> CreeMC-Slot */
    static int toCree(int i) {
        if (i >= 0 && i <= 8) return 36 + i;
        if (i >= 9 && i <= 35) return i;
        return switch (i) { case 36 -> 8; case 37 -> 7; case 38 -> 6; case 39 -> 5; case 40 -> 45; default -> -1; };
    }

    private Map<Integer, ItemStack> snapshot() {
        Map<Integer, ItemStack> out = new HashMap<>();
        Map<String, Object> raw = Json.obj(bridge.request("inventory", "uuid", player.getUniqueId().toString()));
        for (Map.Entry<String, Object> e : raw.entrySet()) {
            List<Object> st = Json.list(e.getValue());
            if (st.size() >= 2) out.put(Integer.parseInt(e.getKey()), ItemStack.fromCree(Json.str(st.get(0), null), (int) Json.num(st.get(1), 1)));
        }
        return out;
    }

    @Override public int getSize() { return 41; }

    @Override
    public ItemStack getItem(int index) {
        ItemStack s = snapshot().get(toCree(index));
        return s == null || s.isEmpty() ? null : s;
    }

    @Override
    public void setItem(int index, ItemStack item) {
        int slot = toCree(index);
        if (slot < 0) throw new ArrayIndexOutOfBoundsException(index);
        if (item == null || item.isEmpty()) bridge.call("set_slot", "to", player.getUniqueId().toString(), "slot", slot, "item", null, "count", 0);
        else bridge.call("set_slot", "to", player.getUniqueId().toString(), "slot", slot, "item", item.toCreeItem(), "count", item.getAmount());
    }

    @Override
    public HashMap<Integer, ItemStack> addItem(ItemStack... items) {
        HashMap<Integer, ItemStack> rest = new HashMap<>();
        for (int i = 0; i < items.length; i++) {
            ItemStack it = items[i];
            if (it == null || it.isEmpty()) continue;
            Object left = bridge.request("give", "uuid", player.getUniqueId().toString(), "item", it.toCreeItem(), "count", it.getAmount());
            int n = (int) Json.num(left, 0);
            if (n > 0) {
                ItemStack r = it.clone();
                r.setAmount(n);
                rest.put(i, r);
            }
        }
        return rest;
    }

    @Override
    public HashMap<Integer, ItemStack> removeItem(ItemStack... items) {
        HashMap<Integer, ItemStack> rest = new HashMap<>();
        for (int i = 0; i < items.length; i++) {
            ItemStack it = items[i];
            if (it == null) continue;
            Object left = bridge.request("take", "uuid", player.getUniqueId().toString(), "item", it.toCreeItem(), "count", it.getAmount());
            int n = (int) Json.num(left, 0);
            if (n > 0) {
                ItemStack r = it.clone();
                r.setAmount(n);
                rest.put(i, r);
            }
        }
        return rest;
    }

    @Override
    public ItemStack[] getContents() {
        Map<Integer, ItemStack> snap = snapshot();
        ItemStack[] out = new ItemStack[41];
        for (int i = 0; i < 41; i++) out[i] = snap.get(toCree(i));
        return out;
    }

    @Override
    public void setContents(ItemStack[] items) {
        clear();
        for (int i = 0; i < Math.min(items.length, 41); i++) if (items[i] != null) setItem(i, items[i]);
    }

    @Override public boolean contains(Material material) { return contains(material, 1); }

    @Override
    public boolean contains(Material material, int amount) {
        int n = 0;
        for (ItemStack s : snapshot().values()) if (s.getType() == material) n += s.getAmount();
        return n >= amount;
    }

    @Override
    public boolean containsAtLeast(ItemStack item, int amount) {
        int n = 0;
        for (ItemStack s : snapshot().values()) if (s.isSimilar(item)) n += s.getAmount();
        return n >= amount;
    }

    @Override
    public int firstEmpty() {
        Map<Integer, ItemStack> snap = snapshot();
        for (int i = 0; i < 36; i++) if (!snap.containsKey(toCree(i))) return i;
        return -1;
    }

    @Override public boolean isEmpty() { return snapshot().isEmpty(); }

    @Override public void clear() { bridge.call("clear", "to", player.getUniqueId().toString()); }

    @Override public void clear(int index) { setItem(index, null); }

    @Override
    public void remove(Material material) {
        Map<Integer, ItemStack> snap = snapshot();
        for (int i = 0; i < 41; i++) {
            ItemStack s = snap.get(toCree(i));
            if (s != null && s.getType() == material) setItem(i, null);
        }
    }

    @Override
    public ItemStack getItemInMainHand() {
        return player.handItem == null ? new ItemStack(Material.AIR, 0) : ItemStack.fromCree(player.handItem, player.handCount);
    }

    @Override
    public void setItemInMainHand(ItemStack item) {
        setItem(player.heldSlot, item);
        player.handItem = item == null || item.isEmpty() ? null : item.toCreeItem();
        player.handCount = item == null ? 0 : item.getAmount();
    }

    @Override
    public ItemStack getItemInOffHand() {
        ItemStack s = getItem(40);
        return s == null ? new ItemStack(Material.AIR, 0) : s;
    }

    @Override public void setItemInOffHand(ItemStack item) { setItem(40, item); }

    @Override public int getHeldItemSlot() { return player.heldSlot; }

    @Override
    public void setHeldItemSlot(int slot) {
        player.heldSlot = slot;
        bridge.call("held_slot", "to", player.getUniqueId().toString(), "slot", slot);
    }

    @Override public ItemStack getHelmet() { return getItem(39); }

    @Override public ItemStack getChestplate() { return getItem(38); }

    @Override public ItemStack getLeggings() { return getItem(37); }

    @Override public ItemStack getBoots() { return getItem(36); }

    @Override public void setHelmet(ItemStack item) { setItem(39, item); }

    @Override public void setChestplate(ItemStack item) { setItem(38, item); }

    @Override public void setLeggings(ItemStack item) { setItem(37, item); }

    @Override public void setBoots(ItemStack item) { setItem(36, item); }

    @Override
    public ItemStack[] getArmorContents() {
        ItemStack[] all = getContents();
        return new ItemStack[]{all[36], all[37], all[38], all[39]};
    }

    @Override
    public Iterator<ItemStack> iterator() {
        List<ItemStack> l = new ArrayList<>(java.util.Arrays.asList(getContents()).subList(0, 36));
        return l.iterator();
    }
}
