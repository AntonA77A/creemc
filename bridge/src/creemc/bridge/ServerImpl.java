package creemc.bridge;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Logger;
import net.kyori.adventure.text.Component;
import org.bukkit.GameMode;
import org.bukkit.OfflinePlayer;
import org.bukkit.Server;
import org.bukkit.World;
import org.bukkit.command.CommandSender;
import org.bukkit.command.ConsoleCommandSender;
import org.bukkit.command.PluginCommand;
import org.bukkit.entity.Entity;
import org.bukkit.entity.Player;
import org.bukkit.event.Event;
import org.bukkit.event.EventPriority;
import org.bukkit.event.Listener;
import org.bukkit.plugin.EventExecutor;
import org.bukkit.plugin.Plugin;
import org.bukkit.plugin.PluginManager;
import org.bukkit.plugin.java.JavaPlugin;
import org.bukkit.scheduler.BukkitScheduler;

public final class ServerImpl implements Server {
    private final Bridge bridge;
    private final Map<String, Object> info;
    private final Map<UUID, PlayerImpl> players = new ConcurrentHashMap<>();
    private final Map<Integer, EntityImpl> entities = new ConcurrentHashMap<>();
    public final WorldImpl world;
    private final ConsoleImpl console;
    private final PluginManager pluginManager;

    ServerImpl(Bridge bridge, Map<String, Object> info) {
        this.bridge = bridge;
        this.info = info;
        this.world = new WorldImpl(bridge, Json.str(info.get("world"), "world"), Json.list(info.get("spawn")),
                (int) Json.num(info.get("min_y"), -64), (int) Json.num(info.get("max_y"), 320));
        this.console = new ConsoleImpl(bridge);
        this.pluginManager = new PluginManagerImpl();
    }

    // ---- Spieler-Spiegel ----

    PlayerImpl updatePlayer(Map<String, Object> snap) {
        UUID id = UUID.fromString(Json.str(snap.get("uuid"), ""));
        PlayerImpl p = players.computeIfAbsent(id, u -> new PlayerImpl(bridge, u));
        p.update(snap);
        return p;
    }

    void syncPlayers(List<Object> snaps) {
        java.util.Set<UUID> seen = new java.util.HashSet<>();
        for (Object o : snaps) seen.add(updatePlayer(Json.obj(o)).getUniqueId());
        players.values().removeIf(p -> {
            if (seen.contains(p.getUniqueId())) return false;
            p.online = false;
            return true;
        });
    }

    void removePlayer(PlayerImpl p) {
        if (p == null) return;
        p.online = false;
        players.remove(p.getUniqueId());
    }

    /** Entity aus einem Schnappschuss {id, type, x, y, z} (Spieler: {uuid, ...}). */
    Entity entity(Map<String, Object> snap) {
        if (snap.get("uuid") != null && snap.get("name") != null) return updatePlayer(snap);
        int id = (int) Json.num(snap.get("id"), -1);
        EntityImpl e = entities.computeIfAbsent(id, i -> new EntityImpl(bridge, i));
        e.update(snap);
        if (entities.size() > 2000) entities.clear();
        return e;
    }

    CommandSender senderFor(String uuid) {
        if (uuid == null) return console;
        PlayerImpl p = players.get(UUID.fromString(uuid));
        return p != null ? p : console;
    }

    // ---- Server ----

    @Override public String getName() { return "CreeMC"; }

    @Override public String getVersion() { return Json.str(info.get("version"), "CreeMC") + " (MC: " + getMinecraftVersion() + ")"; }

    @Override public String getBukkitVersion() { return getMinecraftVersion() + "-R0.1-SNAPSHOT"; }

    @Override public String getMinecraftVersion() { return Json.str(info.get("minecraft"), "1.21.11"); }

    @Override
    public Collection<? extends Player> getOnlinePlayers() { return Collections.unmodifiableList(new ArrayList<>(players.values())); }

    @Override public int getMaxPlayers() { return (int) Json.num(info.get("max_players"), 20); }

    @Override public int getPort() { return (int) Json.num(info.get("port"), 25565); }

    @Override public String getIp() { return Json.str(info.get("host"), ""); }

    @Override public String getMotd() { return Json.str(info.get("motd"), ""); }

    @Override public boolean getOnlineMode() { return Boolean.TRUE.equals(info.get("online_mode")); }

    @Override public int getViewDistance() { return (int) Json.num(info.get("view_distance"), 10); }

    @Override public GameMode getDefaultGameMode() { return GameMode.getByValue((int) Json.num(info.get("gamemode"), 0)); }

    @Override
    public Player getPlayer(String name) {
        Player exact = getPlayerExact(name);
        if (exact != null) return exact;
        String lower = name.toLowerCase();
        Player found = null;
        int delta = Integer.MAX_VALUE;
        for (PlayerImpl p : players.values()) {
            if (p.getName().toLowerCase().startsWith(lower)) {
                int d = p.getName().length() - lower.length();
                if (d < delta) {
                    found = p;
                    delta = d;
                }
            }
        }
        return found;
    }

    @Override
    public Player getPlayerExact(String name) {
        for (PlayerImpl p : players.values()) if (p.getName().equalsIgnoreCase(name)) return p;
        return null;
    }

    @Override public Player getPlayer(UUID id) { return players.get(id); }

    @Override
    public List<Player> matchPlayer(String name) {
        List<Player> out = new ArrayList<>();
        for (PlayerImpl p : players.values()) {
            if (p.getName().equalsIgnoreCase(name)) return List.of(p);
            if (p.getName().toLowerCase().contains(name.toLowerCase())) out.add(p);
        }
        return out;
    }

    @Override
    public OfflinePlayer getOfflinePlayer(String name) {
        Player p = getPlayerExact(name);
        if (p != null) return p;
        Object uuid = bridge.request("uuid_of", "name", name);
        UUID id = uuid != null ? UUID.fromString(uuid.toString())
                : UUID.nameUUIDFromBytes(("OfflinePlayer:" + name).getBytes(java.nio.charset.StandardCharsets.UTF_8));
        return new OfflineImpl(bridge, id, name);
    }

    @Override
    public OfflinePlayer getOfflinePlayer(UUID id) {
        Player p = getPlayer(id);
        return p != null ? p : new OfflineImpl(bridge, id, null);
    }

    @Override public PluginManager getPluginManager() { return pluginManager; }

    @Override public BukkitScheduler getScheduler() { return bridge.scheduler; }

    @Override public World getWorld(String name) { return world.getName().equals(name) ? world : null; }

    @Override public World getWorld(UUID uid) { return world.getUID().equals(uid) ? world : null; }

    @Override public List<World> getWorlds() { return List.of(world); }

    @Override
    public int broadcastMessage(String message) {
        bridge.call("broadcast", "text", message);
        return players.size();
    }

    @Override
    public int broadcast(String message, String permission) {
        int n = 0;
        for (PlayerImpl p : players.values()) {
            if (p.hasPermission(permission)) {
                p.sendMessage(message);
                n++;
            }
        }
        console.sendMessage(message);
        return n;
    }

    @Override public int broadcast(Component message) { return broadcastMessage(message.toLegacy()); }

    @Override public ConsoleCommandSender getConsoleSender() { return console; }

    @Override
    public boolean dispatchCommand(CommandSender sender, String commandLine) {
        if (bridge.commands.dispatch(sender, commandLine)) return true;
        bridge.call("command", "sender", sender instanceof Player p ? p.getUniqueId().toString() : null, "line", commandLine);
        return true;
    }

    @Override public PluginCommand getPluginCommand(String name) { return bridge.commands.findPluginCommand(name); }

    @Override public Logger getLogger() { return Logger.getLogger("Server"); }

    @Override public boolean isPrimaryThread() { return bridge.isMainThread(); }

    @Override public void shutdown() { bridge.call("shutdown"); }

    @Override public double[] getTPS() { return new double[]{20, 20, 20}; }

    @Override public void reload() { getLogger().warning("/reload wird für Java-Plugins nicht unterstützt - Server neu starten"); }

    @Override public void savePlayers() { bridge.call("save"); }

    // ---- PluginManager ----

    private final class PluginManagerImpl implements PluginManager {
        @Override
        public Plugin getPlugin(String name) {
            for (Plugin p : bridge.plugins) if (p.getName().equalsIgnoreCase(name)) return p;
            return null;
        }

        @Override public Plugin[] getPlugins() { return bridge.plugins.toArray(new Plugin[0]); }

        @Override
        public boolean isPluginEnabled(String name) {
            Plugin p = getPlugin(name);
            return p != null && p.isEnabled();
        }

        @Override public boolean isPluginEnabled(Plugin plugin) { return plugin != null && plugin.isEnabled(); }

        @Override public void callEvent(Event event) { bridge.events.call(event); }

        @Override
        public void registerEvents(Listener listener, Plugin plugin) {
            if (!plugin.isEnabled()) throw new IllegalStateException("Plugin attempted to register " + listener + " while not enabled");
            bridge.events.register(listener, plugin);
        }

        @Override
        public void registerEvent(Class<? extends Event> event, Listener listener, EventPriority priority, EventExecutor executor, Plugin plugin) {
            registerEvent(event, listener, priority, executor, plugin, false);
        }

        @Override
        public void registerEvent(Class<? extends Event> event, Listener listener, EventPriority priority, EventExecutor executor,
                                  Plugin plugin, boolean ignoreCancelled) {
            bridge.events.register(event, listener, priority, executor, plugin, ignoreCancelled);
        }

        @Override
        public void disablePlugin(Plugin plugin) {
            if (!(plugin instanceof JavaPlugin jp) || !jp.isEnabled()) return;
            plugin.getLogger().info("Disabling " + plugin.getDescription().getFullName());
            try {
                jp.setEnabled(false);
            } catch (Throwable t) {
                Logger.getLogger("CreeMC-Bridge").log(java.util.logging.Level.SEVERE, "Error disabling " + plugin.getName(), t);
            }
            bridge.scheduler.cancelTasks(plugin);
            Events.unregister(plugin);
        }

        @Override
        public void enablePlugin(Plugin plugin) {
            if (!(plugin instanceof JavaPlugin jp) || jp.isEnabled()) return;
            plugin.getLogger().info("Enabling " + plugin.getDescription().getFullName());
            try {
                jp.setEnabled(true);
                callEvent(new org.bukkit.event.server.PluginEnableEvent(plugin));
            } catch (Throwable t) {
                Logger.getLogger("CreeMC-Bridge").log(java.util.logging.Level.SEVERE,
                        "Error occurred while enabling " + plugin.getDescription().getFullName() + " (Is it up to date?)", t);
                disablePlugin(plugin);
            }
        }
    }

    static Map<String, Object> map(Object... kv) {
        Map<String, Object> m = new LinkedHashMap<>();
        for (int i = 0; i + 1 < kv.length; i += 2) m.put((String) kv[i], kv[i + 1]);
        return m;
    }
    // ---- Inventare, Bossbars, Scoreboards (erweiterte API) ----

    private Scoreboards scoreboards;

    public synchronized Scoreboards scoreboards() {
        if (scoreboards == null) scoreboards = new Scoreboards(bridge);
        return scoreboards;
    }

    @Override public org.bukkit.scoreboard.ScoreboardManager getScoreboardManager() { return scoreboards(); }

    @Override
    public org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, int size) {
        return createInventory(owner, size, (String) null);
    }

    @Override
    public org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, int size, String title) {
        return new GuiInventory(owner, size, title, org.bukkit.event.inventory.InventoryType.CHEST, true);
    }

    @Override
    public org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, org.bukkit.event.inventory.InventoryType type) {
        return createInventory(owner, type, (String) null);
    }

    @Override
    public org.bukkit.inventory.Inventory createInventory(org.bukkit.inventory.InventoryHolder owner, org.bukkit.event.inventory.InventoryType type,
                                                          String title) {
        // CreeMC kann als eigenes Fenster: Kiste (9-54), Trichter, Spender/Werfer - alles andere wird eine Kiste
        org.bukkit.event.inventory.InventoryType t = type.isCreatable() || type == org.bukkit.event.inventory.InventoryType.ENDER_CHEST
                || type == org.bukkit.event.inventory.InventoryType.BARREL || type == org.bukkit.event.inventory.InventoryType.SHULKER_BOX
                ? type : org.bukkit.event.inventory.InventoryType.CHEST;
        int size = t == org.bukkit.event.inventory.InventoryType.HOPPER ? 5
                : t == org.bukkit.event.inventory.InventoryType.DISPENSER || t == org.bukkit.event.inventory.InventoryType.DROPPER ? 9 : 27;
        return new GuiInventory(owner, size, title == null ? type.getDefaultTitle() : title,
                t == org.bukkit.event.inventory.InventoryType.HOPPER || t == org.bukkit.event.inventory.InventoryType.DISPENSER
                        || t == org.bukkit.event.inventory.InventoryType.DROPPER ? t : org.bukkit.event.inventory.InventoryType.CHEST, true);
    }

    @Override
    public org.bukkit.boss.BossBar createBossBar(String title, org.bukkit.boss.BarColor color, org.bukkit.boss.BarStyle style,
                                                 org.bukkit.boss.BarFlag... flags) {
        return new BossBars.Impl(title, color, style, flags);
    }

    static {
        // Adventure-Bossbars: Änderungen an alle Zuschauer schicken
        net.kyori.adventure.bossbar.BossBar.creeSender(a -> {
            var bar = (net.kyori.adventure.bossbar.BossBar) a[0];
            var p = org.bukkit.Bukkit.getPlayer((java.util.UUID) a[1]);
            if (p != null) advBar(p, bar, (int) a[2]);
        });
    }

    static void advBar(org.bukkit.entity.Player p, net.kyori.adventure.bossbar.BossBar bar, int action) {
        BossBars.send(p, bar.creeId(), action, bar.name() == null ? "" : bar.name().toLegacy(), bar.progress(), bar.color().ordinal(),
                bar.overlay().ordinal(), bar.creeFlags());
    }
}