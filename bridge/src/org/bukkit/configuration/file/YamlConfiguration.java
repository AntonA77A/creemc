package org.bukkit.configuration.file;

import java.io.File;
import java.io.Reader;
import java.util.LinkedHashMap;
import java.util.Map;
import org.bukkit.configuration.ConfigurationSection;
import org.bukkit.configuration.InvalidConfigurationException;

public class YamlConfiguration extends FileConfiguration {
    @Override
    public String saveToString() {
        Map<String, Object> data = toMap();
        if (options().copyDefaults() && getDefaults() != null) {
            Map<String, Object> merged = new LinkedHashMap<>();
            mergeDefaults(merged, ((org.bukkit.configuration.MemorySection) getDefaults()).toMap());
            mergeDefaults(merged, data);
            data = merged;
        }
        String header = options().header();
        StringBuilder b = new StringBuilder();
        if (header != null && !header.isEmpty()) for (String l : header.split("\n")) b.append("# ").append(l).append('\n');
        return b + creemc.bridge.Yaml.dump(data);
    }

    @SuppressWarnings("unchecked")
    private static void mergeDefaults(Map<String, Object> into, Map<String, Object> from) {
        for (Map.Entry<String, Object> e : from.entrySet()) {
            if (e.getValue() instanceof Map<?, ?> m && into.get(e.getKey()) instanceof Map<?, ?> existing)
                mergeDefaults((Map<String, Object>) existing, (Map<String, Object>) m);
            else if (e.getValue() instanceof Map<?, ?> m) {
                Map<String, Object> copy = new LinkedHashMap<>();
                mergeDefaults(copy, (Map<String, Object>) m);
                into.put(e.getKey(), copy);
            } else into.put(e.getKey(), e.getValue());
        }
    }

    @Override
    public void loadFromString(String contents) throws InvalidConfigurationException {
        map.clear();
        try {
            for (Map.Entry<String, Object> e : creemc.bridge.Yaml.load(contents).entrySet()) set(e.getKey(), e.getValue());
        } catch (RuntimeException e) {
            throw new InvalidConfigurationException(e);
        }
    }

    public static YamlConfiguration loadConfiguration(File file) {
        YamlConfiguration c = new YamlConfiguration();
        try {
            if (file.exists()) c.load(file);
        } catch (Exception e) {
            java.util.logging.Logger.getLogger("Bukkit").warning("Cannot load " + file + ": " + e);
        }
        return c;
    }

    public static YamlConfiguration loadConfiguration(Reader reader) {
        YamlConfiguration c = new YamlConfiguration();
        try {
            c.load(reader);
        } catch (Exception e) {
            java.util.logging.Logger.getLogger("Bukkit").warning("Cannot load configuration: " + e);
        }
        return c;
    }

    @Override
    public ConfigurationSection getParent() { return null; }
}
