package org.bukkit.plugin;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class PluginDescriptionFile {
    private final Map<String, Object> raw;

    public PluginDescriptionFile(Map<String, Object> raw) { this.raw = raw; }

    public PluginDescriptionFile(String name, String version, String mainClass) {
        this(new LinkedHashMap<>(Map.of("name", name, "version", version, "main", mainClass)));
    }

    private String str(String key) { return raw.get(key) == null ? null : String.valueOf(raw.get(key)); }

    private List<String> list(String key) {
        Object o = raw.get(key);
        List<String> out = new ArrayList<>();
        if (o instanceof List<?> l) for (Object x : l) out.add(String.valueOf(x));
        else if (o != null) out.add(String.valueOf(o));
        return out;
    }

    public String getName() { return str("name"); }

    public String getVersion() { return str("version") == null ? "" : str("version"); }

    public String getFullName() { return getName() + " v" + getVersion(); }

    public String getMain() { return str("main"); }

    public String getDescription() { return str("description"); }

    public String getWebsite() { return str("website"); }

    public String getPrefix() { return str("prefix"); }

    public String getAPIVersion() { return str("api-version"); }

    public List<String> getAuthors() {
        List<String> a = list("authors");
        if (raw.get("author") != null) a.add(0, str("author"));
        return a;
    }

    public List<String> getDepend() { return list("depend"); }

    public List<String> getSoftDepend() { return list("softdepend"); }

    public List<String> getLoadBefore() { return list("loadbefore"); }

    @SuppressWarnings("unchecked")
    public Map<String, Map<String, Object>> getCommands() {
        Map<String, Map<String, Object>> out = new LinkedHashMap<>();
        if (raw.get("commands") instanceof Map<?, ?> m)
            for (Map.Entry<?, ?> e : m.entrySet())
                out.put(String.valueOf(e.getKey()), e.getValue() instanceof Map ? (Map<String, Object>) e.getValue() : new LinkedHashMap<>());
        return out;
    }

    @SuppressWarnings("unchecked")
    public Map<String, Object> getPermissionsRaw() {
        return raw.get("permissions") instanceof Map ? (Map<String, Object>) raw.get("permissions") : Map.of();
    }

    public Map<String, Object> getRaw() { return raw; }
}
