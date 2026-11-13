package org.bukkit.configuration;

import java.util.Map;

public class MemoryConfiguration extends MemorySection implements Configuration {
    protected Configuration defaults;

    public MemoryConfiguration() { }

    public MemoryConfiguration(Configuration defaults) { this.defaults = defaults; }

    @Override
    public void addDefault(String path, Object value) {
        if (defaults == null) defaults = new MemoryConfiguration();
        defaults.set(path, value);
    }

    @Override
    public void addDefaults(Map<String, Object> defs) {
        for (Map.Entry<String, Object> e : defs.entrySet()) addDefault(e.getKey(), e.getValue());
    }

    @Override
    public void addDefaults(Configuration defs) {
        for (Map.Entry<String, Object> e : defs.getValues(true).entrySet())
            if (!(e.getValue() instanceof ConfigurationSection)) addDefault(e.getKey(), e.getValue());
    }

    @Override
    public void setDefaults(Configuration defaults) { this.defaults = defaults; }

    @Override
    public Configuration getDefaults() { return defaults; }

    @Override
    public ConfigurationSection getParent() { return null; }
}
