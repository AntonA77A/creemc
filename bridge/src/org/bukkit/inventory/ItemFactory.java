package org.bukkit.inventory;

import org.bukkit.Material;
import org.bukkit.inventory.meta.ItemMeta;

/** Erzeugt Item-Metadaten (Bukkit.getItemFactory().getItemMeta(Material.DIAMOND_SWORD)). */
public final class ItemFactory {
    public static final ItemFactory INSTANCE = new ItemFactory();

    private ItemFactory() { }

    public ItemMeta getItemMeta(Material material) { return new ItemMeta.Impl(); }

    public boolean isApplicable(ItemMeta meta, Material material) { return true; }

    public boolean equals(ItemMeta a, ItemMeta b) { return java.util.Objects.equals(a, b); }

    public ItemMeta asMetaFor(ItemMeta meta, Material material) { return meta == null ? null : meta.clone(); }
}
