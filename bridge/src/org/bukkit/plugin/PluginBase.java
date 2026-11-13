package org.bukkit.plugin;

public abstract class PluginBase implements Plugin {
    @Override
    public final int hashCode() { return getName().hashCode(); }

    @Override
    public final boolean equals(Object obj) { return obj instanceof Plugin p && getName().equals(p.getName()); }

    @Override
    public final String getName() { return getDescription().getName(); }
}
