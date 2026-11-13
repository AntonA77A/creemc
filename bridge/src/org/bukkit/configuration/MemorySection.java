package org.bukkit.configuration;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class MemorySection implements ConfigurationSection {
    protected final Map<String, Object> map = new LinkedHashMap<>();
    private final Configuration root;
    private final ConfigurationSection parent;
    private final String path;
    private final String fullPath;

    protected MemorySection() {
        this.root = (Configuration) this;
        this.parent = null;
        this.path = "";
        this.fullPath = "";
    }

    protected MemorySection(ConfigurationSection parent, String path) {
        this.parent = parent;
        this.path = path;
        this.root = parent.getRoot();
        this.fullPath = parent.getCurrentPath().isEmpty() ? path : parent.getCurrentPath() + "." + path;
    }

    @Override
    public Set<String> getKeys(boolean deep) {
        Set<String> out = new LinkedHashSet<>();
        Configuration defs = root == null ? null : root.getDefaults();
        if (defs != null && defs != root) {
            ConfigurationSection d = fullPath.isEmpty() ? defs : defs.getConfigurationSection(fullPath);
            if (d != null) out.addAll(d.getKeys(deep));
        }
        collect(out, "", deep);
        return out;
    }

    private void collect(Set<String> out, String prefix, boolean deep) {
        for (Map.Entry<String, Object> e : map.entrySet()) {
            out.add(prefix + e.getKey());
            if (deep && e.getValue() instanceof MemorySection s) s.collect(out, prefix + e.getKey() + ".", true);
        }
    }

    @Override
    public Map<String, Object> getValues(boolean deep) {
        Map<String, Object> out = new LinkedHashMap<>();
        for (String k : getKeys(deep)) out.put(k, get(k));
        return out;
    }

    @Override
    public boolean contains(String path) { return get(path) != null; }

    @Override
    public boolean contains(String path, boolean ignoreDefault) { return ignoreDefault ? getOwn(path) != null : contains(path); }

    @Override
    public boolean isSet(String path) { return getOwn(path) != null; }

    @Override
    public String getCurrentPath() { return fullPath; }

    @Override
    public String getName() { return path; }

    @Override
    public Configuration getRoot() { return root; }

    @Override
    public ConfigurationSection getParent() { return parent; }

    private Object getOwn(String p) {
        if (p.isEmpty()) return this;
        int dot = p.indexOf('.');
        if (dot < 0) return map.get(p);
        Object child = map.get(p.substring(0, dot));
        return child instanceof MemorySection s ? s.getOwn(p.substring(dot + 1)) : null;
    }

    @Override
    public Object get(String path) { return get(path, null); }

    @Override
    public Object get(String p, Object def) {
        Object v = getOwn(p);
        if (v != null) return v;
        if (def != null) return def;
        Configuration defs = root == null ? null : root.getDefaults();
        if (defs != null && defs != root) return defs.get(fullPath.isEmpty() ? p : fullPath + "." + p);
        return null;
    }

    @Override
    public void set(String p, Object value) {
        int dot = p.indexOf('.');
        if (dot >= 0) {
            String head = p.substring(0, dot);
            Object child = map.get(head);
            MemorySection s = child instanceof MemorySection ms ? ms : null;
            if (s == null) {
                if (value == null) return;
                s = new MemorySection(this, head);
                map.put(head, s);
            }
            s.set(p.substring(dot + 1), value);
            return;
        }
        if (value == null) map.remove(p);
        else if (value instanceof Map<?, ?> m) createSection(p, m);
        else map.put(p, value);
    }

    @Override
    public ConfigurationSection createSection(String p) {
        int dot = p.indexOf('.');
        if (dot >= 0) {
            String head = p.substring(0, dot);
            Object child = map.get(head);
            MemorySection s = child instanceof MemorySection ms ? ms : new MemorySection(this, head);
            map.put(head, s);
            return s.createSection(p.substring(dot + 1));
        }
        MemorySection s = new MemorySection(this, p);
        map.put(p, s);
        return s;
    }

    @Override
    public ConfigurationSection createSection(String p, Map<?, ?> values) {
        ConfigurationSection s = createSection(p);
        for (Map.Entry<?, ?> e : values.entrySet()) {
            if (e.getValue() instanceof Map<?, ?> m) s.createSection(String.valueOf(e.getKey()), m);
            else s.set(String.valueOf(e.getKey()), e.getValue());
        }
        return s;
    }

    /** Für das Speichern: verschachtelte Maps statt Sektionen. */
    public Map<String, Object> toMap() {
        Map<String, Object> out = new LinkedHashMap<>();
        for (Map.Entry<String, Object> e : map.entrySet())
            out.put(e.getKey(), e.getValue() instanceof MemorySection s ? s.toMap() : e.getValue());
        return out;
    }

    @Override
    public String getString(String p) { return getString(p, null); }

    @Override
    public String getString(String p, String def) {
        Object v = get(p);
        return v == null || v instanceof MemorySection ? def : String.valueOf(v);
    }

    @Override
    public boolean isString(String p) { return get(p) instanceof String; }

    @Override
    public int getInt(String p) { return getInt(p, 0); }

    @Override
    public int getInt(String p, int def) { return get(p) instanceof Number n ? n.intValue() : parse(get(p), def).intValue(); }

    @Override
    public boolean isInt(String p) { return get(p) instanceof Integer; }

    @Override
    public boolean getBoolean(String p) { return getBoolean(p, false); }

    @Override
    public boolean getBoolean(String p, boolean def) {
        Object v = get(p);
        return v instanceof Boolean b ? b : v instanceof String s && (s.equalsIgnoreCase("true") || s.equalsIgnoreCase("false"))
                ? Boolean.parseBoolean(s) : def;
    }

    @Override
    public boolean isBoolean(String p) { return get(p) instanceof Boolean; }

    @Override
    public double getDouble(String p) { return getDouble(p, 0); }

    @Override
    public double getDouble(String p, double def) { return get(p) instanceof Number n ? n.doubleValue() : parse(get(p), def).doubleValue(); }

    @Override
    public boolean isDouble(String p) { return get(p) instanceof Double; }

    @Override
    public long getLong(String p) { return getLong(p, 0); }

    @Override
    public long getLong(String p, long def) { return get(p) instanceof Number n ? n.longValue() : parse(get(p), def).longValue(); }

    @Override
    public boolean isLong(String p) { return get(p) instanceof Long || get(p) instanceof Integer; }

    private static Number parse(Object v, Number def) {
        if (v instanceof String s) {
            try {
                return Double.parseDouble(s.trim());
            } catch (NumberFormatException ignored) {
            }
        }
        return def;
    }

    @Override
    public List<?> getList(String p) { return getList(p, null); }

    @Override
    public List<?> getList(String p, List<?> def) { return get(p) instanceof List<?> l ? l : def; }

    @Override
    public boolean isList(String p) { return get(p) instanceof List; }

    @Override
    public List<String> getStringList(String p) {
        List<String> out = new ArrayList<>();
        for (Object o : listOrEmpty(p)) if (o != null && !(o instanceof Map)) out.add(String.valueOf(o));
        return out;
    }

    @Override
    public List<Integer> getIntegerList(String p) {
        List<Integer> out = new ArrayList<>();
        for (Object o : listOrEmpty(p)) {
            if (o instanceof Number n) out.add(n.intValue());
            else if (o instanceof String s) try { out.add(Integer.parseInt(s.trim())); } catch (NumberFormatException ignored) { }
        }
        return out;
    }

    @Override
    public List<Double> getDoubleList(String p) {
        List<Double> out = new ArrayList<>();
        for (Object o : listOrEmpty(p)) if (o instanceof Number n) out.add(n.doubleValue());
        return out;
    }

    @Override
    public List<Map<?, ?>> getMapList(String p) {
        List<Map<?, ?>> out = new ArrayList<>();
        for (Object o : listOrEmpty(p)) if (o instanceof Map<?, ?> m) out.add(m);
        return out;
    }

    private List<?> listOrEmpty(String p) { return get(p) instanceof List<?> l ? l : List.of(); }

    @Override
    public ConfigurationSection getConfigurationSection(String p) { return get(p) instanceof ConfigurationSection s ? s : null; }

    @Override
    public boolean isConfigurationSection(String p) { return get(p) instanceof ConfigurationSection; }

    @Override
    public ConfigurationSection getDefaultSection() {
        Configuration defs = root == null ? null : root.getDefaults();
        return defs == null ? null : fullPath.isEmpty() ? defs : defs.getConfigurationSection(fullPath);
    }

    @Override
    public void addDefault(String p, Object value) { root.addDefault(fullPath.isEmpty() ? p : fullPath + "." + p, value); }

    @Override
    public String toString() { return getClass().getSimpleName() + "[path='" + fullPath + "']"; }
}
