package org.bukkit.inventory.meta;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import net.kyori.adventure.text.Component;
import org.bukkit.enchantments.Enchantment;
import org.bukkit.inventory.ItemFlag;

public interface ItemMeta extends Cloneable {
    boolean hasDisplayName();

    String getDisplayName();

    void setDisplayName(String name);

    default Component displayName() { return hasDisplayName() ? Component.text(getDisplayName()) : null; }

    default void displayName(Component name) { setDisplayName(name == null ? null : name.toLegacy()); }

    default boolean hasItemName() { return hasDisplayName(); }

    default void itemName(Component name) { displayName(name); }

    boolean hasLore();

    List<String> getLore();

    void setLore(List<String> lore);

    default List<Component> lore() {
        if (!hasLore()) return null;
        List<Component> out = new ArrayList<>();
        for (String s : getLore()) out.add(Component.text(s));
        return out;
    }

    default void lore(List<? extends Component> lore) {
        if (lore == null) {
            setLore(null);
            return;
        }
        List<String> l = new ArrayList<>();
        for (Component c : lore) l.add(c.toLegacy());
        setLore(l);
    }

    // Verzauberungen
    boolean hasEnchants();

    boolean hasEnchant(Enchantment ench);

    int getEnchantLevel(Enchantment ench);

    Map<Enchantment, Integer> getEnchants();

    boolean addEnchant(Enchantment ench, int level, boolean ignoreLevelRestriction);

    boolean removeEnchant(Enchantment ench);

    default void removeEnchantments() {
        for (Enchantment e : new ArrayList<>(getEnchants().keySet())) removeEnchant(e);
    }

    boolean hasConflictingEnchant(Enchantment ench);

    // Sonstiges
    boolean isUnbreakable();

    void setUnbreakable(boolean unbreakable);

    boolean hasCustomModelData();

    int getCustomModelData();

    void setCustomModelData(Integer data);

    boolean hasEnchantmentGlintOverride();

    Boolean getEnchantmentGlintOverride();

    void setEnchantmentGlintOverride(Boolean override);

    void addItemFlags(ItemFlag... flags);

    void removeItemFlags(ItemFlag... flags);

    Set<ItemFlag> getItemFlags();

    boolean hasItemFlag(ItemFlag flag);

    boolean isEmpty();

    ItemMeta clone();

    class Impl implements ItemMeta {
        private String name;
        private List<String> lore;
        private final Map<Enchantment, Integer> ench = new LinkedHashMap<>();
        private boolean unbreakable;
        private Integer model;
        private Boolean glint;
        private final Set<ItemFlag> flags = EnumSet.noneOf(ItemFlag.class);
        public Map<String, Object> raw;

        @Override public boolean hasDisplayName() { return name != null; }

        @Override public String getDisplayName() { return name; }

        @Override public void setDisplayName(String name) { this.name = name; }

        @Override public boolean hasLore() { return lore != null && !lore.isEmpty(); }

        @Override public List<String> getLore() { return lore == null ? null : new ArrayList<>(lore); }

        @Override public void setLore(List<String> lore) { this.lore = lore == null ? null : new ArrayList<>(lore); }

        @Override public boolean hasEnchants() { return !ench.isEmpty(); }

        @Override public boolean hasEnchant(Enchantment e) { return ench.containsKey(e); }

        @Override public int getEnchantLevel(Enchantment e) { return ench.getOrDefault(e, 0); }

        @Override public Map<Enchantment, Integer> getEnchants() { return new LinkedHashMap<>(ench); }

        @Override
        public boolean addEnchant(Enchantment e, int level, boolean ignore) {
            if (e == null || (!ignore && (level < 1 || level > e.getMaxLevel()))) return false;
            Integer old = ench.put(e, level);
            return old == null || old != level;
        }

        @Override public boolean removeEnchant(Enchantment e) { return ench.remove(e) != null; }

        @Override public boolean hasConflictingEnchant(Enchantment e) { return false; }

        @Override public boolean isUnbreakable() { return unbreakable; }

        @Override public void setUnbreakable(boolean u) { unbreakable = u; }

        @Override public boolean hasCustomModelData() { return model != null; }

        @Override public int getCustomModelData() { return model == null ? 0 : model; }

        @Override public void setCustomModelData(Integer data) { model = data; }

        @Override public boolean hasEnchantmentGlintOverride() { return glint != null; }

        @Override public Boolean getEnchantmentGlintOverride() { return glint; }

        @Override public void setEnchantmentGlintOverride(Boolean o) { glint = o; }

        @Override public void addItemFlags(ItemFlag... f) { flags.addAll(List.of(f)); }

        @Override public void removeItemFlags(ItemFlag... f) { List.of(f).forEach(flags::remove); }

        @Override public Set<ItemFlag> getItemFlags() { return EnumSet.copyOf(flags.isEmpty() ? EnumSet.noneOf(ItemFlag.class) : flags); }

        @Override public boolean hasItemFlag(ItemFlag f) { return flags.contains(f); }

        @Override
        public boolean isEmpty() {
            return name == null && !hasLore() && ench.isEmpty() && !unbreakable && model == null && glint == null && flags.isEmpty();
        }

        @Override
        public ItemMeta clone() {
            Impl c = new Impl();
            c.name = name;
            c.lore = lore == null ? null : new ArrayList<>(lore);
            c.ench.putAll(ench);
            c.unbreakable = unbreakable;
            c.model = model;
            c.glint = glint;
            c.flags.addAll(flags);
            c.raw = raw;
            return c;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof Impl i && java.util.Objects.equals(name, i.name) && java.util.Objects.equals(lore, i.lore)
                    && ench.equals(i.ench) && unbreakable == i.unbreakable && java.util.Objects.equals(model, i.model)
                    && java.util.Objects.equals(glint, i.glint) && flags.equals(i.flags);
        }

        @Override
        public int hashCode() { return java.util.Objects.hash(name, lore, ench, unbreakable, model, glint, flags); }
    }
}
