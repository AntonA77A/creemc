package creemc.bridge;

import java.io.File;
import java.io.InputStream;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.jar.JarFile;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.bukkit.plugin.PluginDescriptionFile;
import org.bukkit.plugin.java.JavaPlugin;

/** Lädt Plugin-Jars aus plugins/ (plugin.yml oder paper-plugin.yml) in Abhängigkeitsreihenfolge. */
final class PluginLoader {
    private static final Logger LOG = Logger.getLogger("CreeMC-Bridge");
    private final Bridge bridge;
    private final List<PluginClassLoader> loaders = new ArrayList<>();

    PluginLoader(Bridge bridge) { this.bridge = bridge; }

    final class PluginClassLoader extends URLClassLoader {
        PluginClassLoader(File jar) throws Exception {
            super(new URL[]{jar.toURI().toURL()}, Bridge.class.getClassLoader());
        }

        Class<?> findOwn(String name) throws ClassNotFoundException {
            synchronized (getClassLoadingLock(name)) {
                Class<?> c = findLoadedClass(name);
                return c != null ? c : findClass(name);
            }
        }

        @Override
        protected Class<?> loadClass(String name, boolean resolve) throws ClassNotFoundException {
            try {
                return super.loadClass(name, resolve);
            } catch (ClassNotFoundException e) {
                for (PluginClassLoader other : loaders) {   // Klassen anderer Plugins (depend/softdepend)
                    if (other == this) continue;
                    try {
                        return other.findOwn(name);
                    } catch (ClassNotFoundException ignored) {
                    }
                }
                throw e;
            }
        }
    }

    void loadAll(File dir) {
        File[] jars = dir.listFiles((d, n) -> n.toLowerCase().endsWith(".jar"));
        if (jars == null) return;
        Map<String, Object[]> found = new LinkedHashMap<>();   // Name -> {Beschreibung, Jar}
        for (File jar : jars) {
            try (JarFile jf = new JarFile(jar)) {
                var entry = jf.getJarEntry("plugin.yml");
                boolean paper = false;
                if (entry == null) {
                    entry = jf.getJarEntry("paper-plugin.yml");
                    paper = entry != null;
                }
                if (entry == null) entry = jf.getJarEntry("creemc.yml");   // eigenes CreeMC-Plugin
                if (entry == null) {
                    LOG.warning(jar.getName() + ": keine plugin.yml/creemc.yml - kein Plugin,übersprungen");
                    continue;
                }
                String yml;
                try (InputStream in = jf.getInputStream(entry)) {
                    yml = new String(in.readAllBytes(), StandardCharsets.UTF_8);
                }
                PluginDescriptionFile desc = new PluginDescriptionFile(Yaml.load(yml));
                if (desc.getName() == null || desc.getMain() == null) {
                    LOG.warning(jar.getName() + ": plugin.yml ohne name/main");
                    continue;
                }
                if (paper && desc.getRaw().containsKey("bootstrapper"))
                    LOG.warning(desc.getName() + ": Paper-Bootstrapper werden nicht unterstützt (wird ignoriert)");
                if (desc.getRaw().containsKey("libraries"))
                    LOG.warning(desc.getName() + ": 'libraries' aus plugin.yml werden nicht heruntergeladen");
                if (found.containsKey(desc.getName())) {
                    LOG.warning("Plugin '" + desc.getName() + "' ist doppelt vorhanden (" + jar.getName() + ") - übersprungen");
                    continue;
                }
                found.put(desc.getName(), new Object[]{desc, jar});
            } catch (Exception e) {
                LOG.log(Level.SEVERE, "Kann " + jar.getName() + " nicht lesen", e);
            }
        }

        List<String> order = new ArrayList<>();
        for (String name : found.keySet()) visit(name, found, order, new ArrayList<>());

        List<JavaPlugin> loaded = new ArrayList<>();
        for (String name : order) {
            PluginDescriptionFile desc = (PluginDescriptionFile) found.get(name)[0];
            File jar = (File) found.get(name)[1];
            List<String> missing = new ArrayList<>();
            for (String dep : desc.getDepend()) if (!isLoaded(dep, loaded)) missing.add(dep);
            if (!missing.isEmpty()) {
                LOG.severe(name + " braucht " + missing + " - nicht geladen");
                continue;
            }
            try {
                PluginClassLoader loader = new PluginClassLoader(jar);
                loaders.add(loader);
                Class<?> main = Class.forName(desc.getMain(), true, loader);
                File data = new File(jar.getParentFile(), name);
                JavaPlugin.INIT.set(new Object[]{desc, data, jar, loader});
                JavaPlugin plugin;
                try {
                    plugin = (JavaPlugin) main.getDeclaredConstructor().newInstance();
                } finally {
                    JavaPlugin.INIT.remove();
                }
                bridge.plugins.add(plugin);
                bridge.commands.registerFromDescription(plugin);
                LOG.info("Lade " + desc.getFullName());
                plugin.onLoad();
                loaded.add(plugin);
            } catch (Throwable t) {
                LOG.log(Level.SEVERE, "Plugin " + name + " konnte nicht geladen werden", t);
            }
        }
        for (JavaPlugin plugin : loaded) bridge.server.getPluginManager().enablePlugin(plugin);
    }

    private static boolean isLoaded(String name, List<JavaPlugin> loaded) {
        for (JavaPlugin p : loaded) if (p.getName().equals(name)) return true;
        return false;
    }

    private static void visit(String name, Map<String, Object[]> found, List<String> order, List<String> stack) {
        if (order.contains(name) || !found.containsKey(name) || stack.contains(name)) return;
        stack.add(name);
        PluginDescriptionFile d = (PluginDescriptionFile) found.get(name)[0];
        for (String dep : d.getDepend()) visit(dep, found, order, stack);
        for (String dep : d.getSoftDepend()) visit(dep, found, order, stack);
        for (Map.Entry<String, Object[]> e : found.entrySet())   // loadbefore: andere Plugins, die uns vorher wollen
            if (((PluginDescriptionFile) e.getValue()[0]).getLoadBefore().contains(name)) visit(e.getKey(), found, order, stack);
        stack.remove(name);
        if (!order.contains(name)) order.add(name);
    }
}
