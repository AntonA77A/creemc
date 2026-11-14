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
        return o != null && o.type == type && o.getItemMeta().equals(getItemMeta());
    }

    // ---- Verzauberungen ----

    public void addEnchantment(org.bukkit.enchantments.Enchantment e, int level) {
        if (level < 1 || level > e.getMaxLevel()) throw new IllegalArgumentException("Stufe " + level + " ist für " + e.getKey() + " nicht erlaubt");
        addUnsafeEnchantment(e, level);
    }

    public void addUnsafeEnchantment(org.bukkit.enchantments.Enchantment e, int level) {
        ItemMeta m = getItemMeta();
        m.addEnchant(e, level, true);
        setItemMeta(m);
    }

    public void addEnchantments(Map<org.bukkit.enchantments.Enchantment, Integer> map) { map.forEach(this::addEnchantment); }

    public void addUnsafeEnchantments(Map<org.bukkit.enchantments.Enchantment, Integer> map) { map.forEach(this::addUnsafeEnchantment); }

    public int removeEnchantment(org.bukkit.enchantments.Enchantment e) {
        ItemMeta m = getItemMeta();
        int old = m.getEnchantLevel(e);
        m.removeEnchant(e);
        setItemMeta(m);
        return old;
    }

    public boolean containsEnchantment(org.bukkit.enchantments.Enchantment e) { return getItemMeta().hasEnchant(e); }

    public int getEnchantmentLevel(org.bukkit.enchantments.Enchantment e) { return getItemMeta().getEnchantLevel(e); }

    public Map<org.bukkit.enchantments.Enchantment, Integer> getEnchantments() { return getItemMeta().getEnchants(); }

    public ItemStack add(int n) {
        amount += n;
        return this;
    }

    public ItemStack subtract(int n) {
        amount -= n;
        return this;
    }

    public ItemStack asQuantity(int n) {
        ItemStack c = clone();
        c.amount = n;
        return c;
    }

    public ItemStack asOne() { return asQuantity(1); }

    public net.kyori.adventure.text.Component displayName() {
        ItemMeta m = getItemMeta();
        return m.hasDisplayName() ? m.displayName() : net.kyori.adventure.text.Component.text(type.name().toLowerCase().replace('_', ' '));
    }

    /** Paper: Meta direkt bearbeiten, z.B. stack.editMeta(m -&gt; m.setDisplayName("x")). */
    public boolean editMeta(java.util.function.Consumer<? super ItemMeta> edit) {
        ItemMeta m = getItemMeta();
        edit.accept(m);
        return setItemMeta(m);
    }

    public void lore(List<? extends net.kyori.adventure.text.Component> lore) { editMeta(m -> m.lore(lore)); }

    public List<net.kyori.adventure.text.Component> lore() { return getItemMeta().lore(); }

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
        // Schlüssel sortiert (TreeMap) wie im Server, damit gleiche Items stapeln
        Map<String, Object> m = new java.util.TreeMap<>();
        if (meta instanceof ItemMeta.Impl impl && impl.raw != null) m.putAll(impl.raw);   // Unbekanntes (z.B. repair) behalten
        for (String k : List.of("name", "lore", "ench", "stored", "unbreakable", "model", "glint", "flags")) m.remove(k);
        if (meta.hasDisplayName()) m.put("name", meta.getDisplayName());
        if (meta.hasLore()) m.put("lore", meta.getLore());
        if (meta.hasEnchants()) {
            Map<String, Object> e = new java.util.TreeMap<>();
            meta.getEnchants().forEach((en, lvl) -> e.put(en.getKey().asString(), lvl));
            m.put(type == Material.ENCHANTED_BOOK ? "stored" : "ench", e);
        }
        if (meta.isUnbreakable()) m.put("unbreakable", true);
        if (meta.hasCustomModelData()) m.put("model", meta.getCustomModelData());
        if (meta.hasEnchantmentGlintOverride()) m.put("glint", meta.getEnchantmentGlintOverride());
        if (!meta.getItemFlags().isEmpty()) {
            List<String> f = new ArrayList<>();
            for (org.bukkit.inventory.ItemFlag fl : meta.getItemFlags()) f.add(fl.name());
            java.util.Collections.sort(f);
            m.put("flags", f);
        }
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
            Object en = m.get("ench") != null ? m.get("ench") : m.get("stored");
            if (en instanceof Map<?, ?> em)
                em.forEach((k, v) -> {
                    org.bukkit.enchantments.Enchantment e = org.bukkit.enchantments.Enchantment.getByKey(org.bukkit.NamespacedKey.fromString(String.valueOf(k)));
                    if (e != null) meta.addEnchant(e, (int) creemc.bridge.Json.num(v, 1), true);
                });
            if (Boolean.TRUE.equals(m.get("unbreakable"))) meta.setUnbreakable(true);
            if (m.get("model") instanceof Number n) meta.setCustomModelData(n.intValue());
            if (m.get("glint") instanceof Boolean g) meta.setEnchantmentGlintOverride(g);
            if (m.get("flags") instanceof List<?> fl)
                for (Object o : fl)
                    try {
                        meta.addItemFlags(org.bukkit.inventory.ItemFlag.valueOf(String.valueOf(o)));
                    } catch (IllegalArgumentException ignored) {
                    }
            ((ItemMeta.Impl) meta).raw = m;
            s.meta = meta;
        }
        return s;
    }
}
