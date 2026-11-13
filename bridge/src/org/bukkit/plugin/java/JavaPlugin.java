package org.bukkit.plugin.java;

import io.papermc.paper.command.brigadier.BasicCommand;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.net.URL;
import java.net.URLConnection;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.Collection;
import java.util.List;
import java.util.logging.Logger;
import org.bukkit.Server;
import org.bukkit.command.Command;
import org.bukkit.command.CommandSender;
import org.bukkit.command.PluginCommand;
import org.bukkit.configuration.file.FileConfiguration;
import org.bukkit.configuration.file.YamlConfiguration;
import org.bukkit.plugin.PluginBase;
import org.bukkit.plugin.PluginDescriptionFile;

public abstract class JavaPlugin extends PluginBase {
    /** Wird vom CreeMC-Loader vor dem Konstruktor gesetzt (wie Bukkits PluginClassLoader). */
    public static final ThreadLocal<Object[]> INIT = new ThreadLocal<>();

    private boolean enabled;
    private PluginDescriptionFile description;
    private File dataFolder;
    private File file;
    private ClassLoader classLoader;
    private Logger logger;
    private FileConfiguration newConfig;
    private File configFile;
    private boolean naggable = true;

    public JavaPlugin() {
        Object[] init = INIT.get();
        if (init != null) init((PluginDescriptionFile) init[0], (File) init[1], (File) init[2], (ClassLoader) init[3]);
    }

    public final void init(PluginDescriptionFile description, File dataFolder, File file, ClassLoader loader) {
        this.description = description;
        this.dataFolder = dataFolder;
        this.file = file;
        this.classLoader = loader;
        this.configFile = new File(dataFolder, "config.yml");
        this.logger = creemc.bridge.Bridge.pluginLogger(description.getPrefix() != null ? description.getPrefix() : description.getName());
    }

    @Override
    public final File getDataFolder() { return dataFolder; }

    @Override
    public final PluginDescriptionFile getDescription() { return description; }

    @SuppressWarnings("deprecation")
    public final PluginDescriptionFile getPluginMeta() { return description; }

    protected File getFile() { return file; }

    protected final ClassLoader getClassLoader() { return classLoader; }

    @Override
    public final Server getServer() { return org.bukkit.Bukkit.getServer(); }

    @Override
    public final boolean isEnabled() { return enabled; }

    public final void setEnabled(boolean value) {
        if (enabled == value) return;
        enabled = value;
        if (value) onEnable(); else onDisable();
    }

    @Override
    public Logger getLogger() { return logger; }

    public org.slf4j.Logger getSLF4JLogger() { return new creemc.bridge.Slf4j(logger); }

    public net.kyori.adventure.text.logger.slf4j.ComponentLogger getComponentLogger() { return new creemc.bridge.Slf4j(logger); }

    @Override
    public FileConfiguration getConfig() {
        if (newConfig == null) reloadConfig();
        return newConfig;
    }

    protected final Reader getTextResource(String file) {
        InputStream in = getResource(file);
        return in == null ? null : new InputStreamReader(in, StandardCharsets.UTF_8);
    }

    @Override
    public void reloadConfig() {
        newConfig = YamlConfiguration.loadConfiguration(configFile);
        InputStream defs = getResource("config.yml");
        if (defs != null) newConfig.setDefaults(YamlConfiguration.loadConfiguration(new InputStreamReader(defs, StandardCharsets.UTF_8)));
    }

    @Override
    public void saveConfig() {
        try {
            getConfig().save(configFile);
        } catch (IOException e) {
            logger.severe("Could not save config to " + configFile + ": " + e);
        }
    }

    @Override
    public void saveDefaultConfig() {
        if (!configFile.exists()) saveResource("config.yml", false);
    }

    @Override
    public void saveResource(String resourcePath, boolean replace) {
        if (resourcePath == null || resourcePath.isEmpty()) throw new IllegalArgumentException("ResourcePath cannot be null or empty");
        resourcePath = resourcePath.replace('\\', '/');
        InputStream in = getResource(resourcePath);
        if (in == null) throw new IllegalArgumentException("The embedded resource '" + resourcePath + "' cannot be found in " + file);
        File out = new File(dataFolder, resourcePath);
        try (in) {
            out.getParentFile().mkdirs();
            if (!out.exists() || replace) Files.copy(in, out.toPath(), StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException e) {
            logger.severe("Could not save " + out.getName() + " to " + out + ": " + e);
        }
    }

    @Override
    public InputStream getResource(String filename) {
        try {
            URL url = classLoader.getResource(filename);
            if (url == null) return null;
            URLConnection c = url.openConnection();
            c.setUseCaches(false);
            return c.getInputStream();
        } catch (IOException e) {
            return null;
        }
    }

    public PluginCommand getCommand(String name) { return creemc.bridge.Bridge.get().commands.pluginCommand(this, name); }

    /** Paper: Befehl ohne plugin.yml-Eintrag. */
    public void registerCommand(String label, BasicCommand command) { registerCommand(label, null, List.of(), command); }

    public void registerCommand(String label, String description, BasicCommand command) { registerCommand(label, description, List.of(), command); }

    public void registerCommand(String label, Collection<String> aliases, BasicCommand command) { registerCommand(label, null, aliases, command); }

    public void registerCommand(String label, String description, Collection<String> aliases, BasicCommand command) {
        creemc.bridge.Bridge.get().commands.registerBasic(this, label, description, aliases, command);
    }

    @Override
    public boolean onCommand(CommandSender sender, Command command, String label, String[] args) { return false; }

    @Override
    public List<String> onTabComplete(CommandSender sender, Command command, String alias, String[] args) { return null; }

    @Override
    public void onLoad() { }

    @Override
    public void onDisable() { }

    @Override
    public void onEnable() { }

    @Override
    public final boolean isNaggable() { return naggable; }

    @Override
    public final void setNaggable(boolean canNag) { naggable = canNag; }

    @Override
    public String toString() { return description.getFullName(); }

    public static <T extends JavaPlugin> T getPlugin(Class<T> clazz) {
        for (org.bukkit.plugin.Plugin p : creemc.bridge.Bridge.get().plugins)
            if (clazz.isInstance(p)) return clazz.cast(p);
        throw new IllegalArgumentException(clazz + " is not provided by a loaded plugin");
    }

    public static JavaPlugin getProvidingPlugin(Class<?> clazz) {
        for (org.bukkit.plugin.Plugin p : creemc.bridge.Bridge.get().plugins)
            if (p instanceof JavaPlugin jp && jp.classLoader == clazz.getClassLoader()) return jp;
        throw new IllegalArgumentException(clazz + " is not provided by a loaded plugin");
    }
}
