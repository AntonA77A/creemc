package org.bukkit.inventory;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.bukkit.Material;
import org.bukkit.inventory.meta.ItemMeta;

public class ItemStack implements Cloneable {
    private Material type;
    private int amount;
    private ItemMeta meta;

    public ItemStack(Material type) { this(type, 1); }

    public ItemStack(Material type, int amount) {
        this.type = type == null ? Material.AIR : type;
        this.amount = amount;
    }

    public ItemStack(ItemStack other) {
        this(other.type, other.amount);
        if (other.meta != null) meta = other.meta.clone();
    }

    public static ItemStack of(Material type) { return new ItemStack(type); }

    public static ItemStack of(Material type, int amount) { return new ItemStack(type, amount); }

    public static ItemStack empty() { return new ItemStack(Material.AIR, 0); }

    public Material getType() { return type; }

    public void setType(Material type) { this.type = type; }

    public int getAmount() { return amount; }

    public void setAmount(int amount) { this.amount = amount; }

    public int getMaxStackSize() { return type.getMaxStackSize(); }

    public boolean isEmpty() { return type == Material.AIR || amount <= 0; }

    public boolean hasItemMeta() { return meta != null && !meta.isEmpty(); }

    public ItemMeta getItemMeta() { return meta != null ? meta.clone() : new ItemMeta.Impl(); }

    public boolean setItemMeta(ItemMeta itemMeta) {
        meta = itemMeta == null ? null : itemMeta.clone();
        return true;
    }

    public boolean isSimilar(ItemStack o) {
        return o != null && o.type == type && java.util.Objects.equals(o.getItemMeta().getDisplayName(), getItemMeta().getDisplayName());
    }

    @Override
    public ItemStack clone() { return new ItemStack(this); }

    @Override
    public boolean equals(Object o) { return o instanceof ItemStack s && isSimilar(s) && s.amount == amount; }

    @Override
    public int hashCode() { return type.hashCode() * 31 + amount; }

    @Override
    public String toString() { return "ItemStack{" + type + " x " + amount + "}"; }

    // ---- CreeMC: Umwandlung in/aus "minecraft:diamond_sword@{...}" ----

    public String toCreeItem() {
        String base = type.getKey().asString();
        if (meta == null) return base;
        Map<String, Object> m = new LinkedHashMap<>();
        if (meta instanceof ItemMeta.Impl impl && impl.raw != null) m.putAll(impl.raw);   // Verzauberungen usw. behalten
        m.remove("name");
        m.remove("lore");
        if (meta.hasDisplayName()) m.put("name", meta.getDisplayName());
        if (meta.hasLore()) m.put("lore", meta.getLore());
        return m.isEmpty() ? base : base + "@" + creemc.bridge.Json.write(m);
    }

    @SuppressWarnings("unchecked")
    public static ItemStack fromCree(String item, int count) {
        if (item == null) return new ItemStack(Material.AIR, 0);
        int at = item.indexOf('@');
        ItemStack s = new ItemStack(Material.fromKey(at < 0 ? item : item.substring(0, at)), count);
        if (at >= 0) {
            Map<String, Object> m = creemc.bridge.Json.obj(creemc.bridge.Json.parse(item.substring(at + 1)));
            ItemMeta meta = new ItemMeta.Impl();
            if (m.get("name") instanceof String n) meta.setDisplayName(n);
            if (m.get("lore") instanceof List<?> l) {
                List<String> lore = new ArrayList<>();
                for (Object o : l) lore.add(String.valueOf(o));
                meta.setLore(lore);
            }
            ((ItemMeta.Impl) meta).raw = m;
            s.meta = meta;
        }
        return s;
    }
}
