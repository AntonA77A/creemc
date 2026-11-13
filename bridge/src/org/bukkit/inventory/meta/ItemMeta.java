package org.bukkit.inventory.meta;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import net.kyori.adventure.text.Component;

public interface ItemMeta extends Cloneable {
    boolean hasDisplayName();

    String getDisplayName();

    void setDisplayName(String name);

    default Component displayName() { return hasDisplayName() ? Component.text(getDisplayName()) : null; }

    default void displayName(Component name) { setDisplayName(name == null ? null : name.toLegacy()); }

    boolean hasLore();

    List<String> getLore();

    void setLore(List<String> lore);

    default void lore(List<? extends Component> lore) {
        if (lore == null) {
            setLore(null);
            return;
        }
        List<String> l = new ArrayList<>();
        for (Component c : lore) l.add(c.toLegacy());
        setLore(l);
    }

    boolean isEmpty();

    ItemMeta clone();

    class Impl implements ItemMeta {
        private String name;
        private List<String> lore;
        public Map<String, Object> raw;

        @Override
        public boolean hasDisplayName() { return name != null; }

        @Override
        public String getDisplayName() { return name; }

        @Override
        public void setDisplayName(String name) { this.name = name; }

        @Override
        public boolean hasLore() { return lore != null && !lore.isEmpty(); }

        @Override
        public List<String> getLore() { return lore == null ? null : new ArrayList<>(lore); }

        @Override
        public void setLore(List<String> lore) { this.lore = lore == null ? null : new ArrayList<>(lore); }

        @Override
        public boolean isEmpty() { return name == null && !hasLore(); }

        @Override
        public ItemMeta clone() {
            Impl c = new Impl();
            c.name = name;
            c.lore = lore == null ? null : new ArrayList<>(lore);
            c.raw = raw;
            return c;
        }
    }
}
