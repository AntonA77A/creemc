package org.bukkit;

import org.bukkit.plugin.Plugin;

public final class NamespacedKey {
    public static final String MINECRAFT = "minecraft";
    private final String namespace, key;

    public NamespacedKey(String namespace, String key) {
        this.namespace = namespace.toLowerCase();
        this.key = key.toLowerCase();
    }

    public NamespacedKey(Plugin plugin, String key) { this(plugin.getName(), key); }

    public static NamespacedKey minecraft(String key) { return new NamespacedKey(MINECRAFT, key); }

    public static NamespacedKey fromString(String s) {
        int i = s.indexOf(':');
        return i < 0 ? minecraft(s) : new NamespacedKey(s.substring(0, i), s.substring(i + 1));
    }

    public String getNamespace() { return namespace; }

    public String getKey() { return key; }

    public String asString() { return namespace + ":" + key; }

    @Override
    public String toString() { return asString(); }

    @Override
    public boolean equals(Object o) { return o instanceof NamespacedKey k && k.namespace.equals(namespace) && k.key.equals(key); }

    @Override
    public int hashCode() { return asString().hashCode(); }
}
