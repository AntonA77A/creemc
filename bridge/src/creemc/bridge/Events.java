package creemc.bridge;

import io.papermc.paper.event.player.AsyncChatEvent;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import net.kyori.adventure.audience.Audience;
import net.kyori.adventure.text.Component;
import org.bukkit.GameMode;
import org.bukkit.Location;
import org.bukkit.block.Block;
import org.bukkit.block.BlockFace;
import org.bukkit.entity.Entity;
import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.Event;
import org.bukkit.event.EventHandler;
import org.bukkit.event.EventPriority;
import org.bukkit.event.Listener;
import org.bukkit.event.block.Action;
import org.bukkit.event.block.BlockBreakEvent;
import org.bukkit.event.block.BlockPlaceEvent;
import org.bukkit.event.entity.EntityDamageByEntityEvent;
import org.bukkit.event.entity.EntityDamageEvent;
import org.bukkit.event.entity.EntityDamageEvent.DamageCause;
import org.bukkit.event.entity.EntityDeathEvent;
import org.bukkit.event.entity.PlayerDeathEvent;
import org.bukkit.event.player.AsyncPlayerChatEvent;
import org.bukkit.event.player.AsyncPlayerPreLoginEvent;
import org.bukkit.event.player.PlayerCommandPreprocessEvent;
import org.bukkit.event.player.PlayerGameModeChangeEvent;
import org.bukkit.event.player.PlayerInteractEntityEvent;
import org.bukkit.event.player.PlayerInteractEvent;
import org.bukkit.event.player.PlayerJoinEvent;
import org.bukkit.event.player.PlayerMoveEvent;
import org.bukkit.event.player.PlayerQuitEvent;
import org.bukkit.event.player.PlayerRespawnEvent;
import org.bukkit.event.player.PlayerToggleSneakEvent;
import org.bukkit.event.player.PlayerToggleSprintEvent;
import org.bukkit.event.server.ServerListPingEvent;
import org.bukkit.inventory.EquipmentSlot;
import org.bukkit.inventory.ItemStack;
import org.bukkit.plugin.EventExecutor;
import org.bukkit.plugin.Plugin;

/** Event-Verteilung an Java-Plugins und Übersetzung der CreeMC-Events. */
public final class Events {
    private static final Logger LOG = Logger.getLogger("CreeMC-Bridge");

    /** Bukkit-Event-Klasse -> CreeMC-Eventnamen, die dafür weitergeleitet werden müssen. */
    private static final Map<Class<? extends Event>, List<String>> FORWARD = new LinkedHashMap<>();

    static {
        FORWARD.put(PlayerJoinEvent.class, List.of("player_join"));
        FORWARD.put(PlayerQuitEvent.class, List.of("player_quit"));
        FORWARD.put(AsyncPlayerChatEvent.class, List.of("player_chat"));
        FORWARD.put(AsyncChatEvent.class, List.of("player_chat"));
        FORWARD.put(PlayerCommandPreprocessEvent.class, List.of("player_command"));
        FORWARD.put(PlayerMoveEvent.class, List.of("player_move"));
        FORWARD.put(BlockBreakEvent.class, List.of("block_break"));
        FORWARD.put(BlockPlaceEvent.class, List.of("block_place"));
        FORWARD.put(PlayerInteractEvent.class, List.of("player_interact"));
        FORWARD.put(PlayerInteractEntityEvent.class, List.of("player_interact_entity"));
        FORWARD.put(PlayerToggleSneakEvent.class, List.of("player_toggle_sneak"));
        FORWARD.put(PlayerToggleSprintEvent.class, List.of("player_toggle_sprint"));
        FORWARD.put(PlayerGameModeChangeEvent.class, List.of("player_gamemode_change"));
        FORWARD.put(EntityDamageEvent.class, List.of("player_damage", "entity_damage"));
        FORWARD.put(EntityDamageByEntityEvent.class, List.of("player_damage", "entity_damage"));
        FORWARD.put(PlayerDeathEvent.class, List.of("player_death"));
        FORWARD.put(EntityDeathEvent.class, List.of("entity_death"));
        FORWARD.put(PlayerRespawnEvent.class, List.of("player_respawn"));
        FORWARD.put(AsyncPlayerPreLoginEvent.class, List.of("player_pre_login"));
        FORWARD.put(ServerListPingEvent.class, List.of("server_list_ping"));
        // erweiterte API
        FORWARD.put(org.bukkit.event.inventory.InventoryClickEvent.class, List.of("inventory_click"));
        FORWARD.put(org.bukkit.event.inventory.InventoryOpenEvent.class, List.of("inventory_open"));
        FORWARD.put(org.bukkit.event.inventory.InventoryCloseEvent.class, List.of("inventory_close"));
        FORWARD.put(org.bukkit.event.player.PlayerDropItemEvent.class, List.of("player_drop_item"));
        FORWARD.put(org.bukkit.event.entity.EntityPickupItemEvent.class, List.of("player_pickup_item"));
        FORWARD.put(org.bukkit.event.player.PlayerItemConsumeEvent.class, List.of("player_item_consume"));
        FORWARD.put(org.bukkit.event.entity.FoodLevelChangeEvent.class, List.of("food_level_change"));
        FORWARD.put(org.bukkit.event.player.PlayerItemHeldEvent.class, List.of("player_item_held"));
        FORWARD.put(org.bukkit.event.player.PlayerSwapHandItemsEvent.class, List.of("player_swap_hand_items"));
        FORWARD.put(org.bukkit.event.player.PlayerToggleFlightEvent.class, List.of("player_toggle_flight"));
        FORWARD.put(org.bukkit.event.player.PlayerKickEvent.class, List.of("player_kick"));
        FORWARD.put(org.bukkit.event.entity.EntityExplodeEvent.class, List.of("entity_explode"));
        FORWARD.put(org.bukkit.event.entity.CreatureSpawnEvent.class, List.of("creature_spawn"));
    }

    private record Handler(Plugin plugin, Listener listener, Class<? extends Event> type, EventPriority priority,
                           boolean ignoreCancelled, EventExecutor executor, long order) { }

    private final Bridge bridge;
    private final List<Handler> handlers = new ArrayList<>();
    private long counter;
    private Set<String> lastForwarded = new HashSet<>();

    Events(Bridge bridge) { this.bridge = bridge; }

    // ---- Registrierung ----

    @SuppressWarnings("unchecked")
    public synchronized void register(Listener listener, Plugin plugin) {
        Set<Method> methods = new LinkedHashSet<>();
        for (Class<?> c = listener.getClass(); c != null && c != Object.class; c = c.getSuperclass())
            for (Method m : c.getDeclaredMethods()) methods.add(m);
        for (Method m : listener.getClass().getMethods()) methods.add(m);
        Set<String> seen = new HashSet<>();
        for (Method m : methods) {
            EventHandler eh = m.getAnnotation(EventHandler.class);
            if (eh == null || m.isBridge() || m.isSynthetic() || m.getParameterCount() != 1
                    || !Event.class.isAssignableFrom(m.getParameterTypes()[0])) continue;
            if (!seen.add(m.getName() + m.getParameterTypes()[0].getName())) continue;
            m.setAccessible(true);
            Class<? extends Event> type = (Class<? extends Event>) m.getParameterTypes()[0];
            EventExecutor ex = (l, e) -> {
                try {
                    m.invoke(l, e);
                } catch (InvocationTargetException ite) {
                    throw new org.bukkit.event.EventException(ite.getCause());
                } catch (IllegalAccessException iae) {
                    throw new org.bukkit.event.EventException(iae);
                }
            };
            handlers.add(new Handler(plugin, listener, type, eh.priority(), eh.ignoreCancelled(), ex, counter++));
        }
        changed();
    }

    public synchronized void register(Class<? extends Event> type, Listener listener, EventPriority priority,
                                      EventExecutor executor, Plugin plugin, boolean ignoreCancelled) {
        handlers.add(new Handler(plugin, listener, type, priority, ignoreCancelled, executor, counter++));
        changed();
    }

    public static void unregister(Object pluginOrListener) {
        Bridge b = Bridge.get();
        if (b == null) return;
        synchronized (b.events) {
            b.events.handlers.removeIf(h -> h.plugin == pluginOrListener || h.listener == pluginOrListener);
            b.events.changed();
        }
    }

    private void changed() {
        if (!bridge.commands.announced) return;   // vor "ready" wird alles auf einmal gemeldet
        Set<String> now = new HashSet<>(names());
        if (!now.equals(lastForwarded)) {   // später angemeldete Handler (z.B. GUIs) dem Server melden
            lastForwarded = now;
            bridge.call("listen", "events", new ArrayList<>(now));
        }
    }

    /** CreeMC-Eventnamen, für die es Java-Handler gibt. */
    public synchronized List<String> forwardedNames() {
        List<String> out = names();
        lastForwarded = new HashSet<>(out);
        return out;
    }

    private synchronized List<String> names() {
        Set<String> out = new LinkedHashSet<>(List.of("player_join", "player_quit", "inventory_close"));
        for (Handler h : handlers)
            for (Map.Entry<Class<? extends Event>, List<String>> e : FORWARD.entrySet())
                if (h.type.isAssignableFrom(e.getKey()) || e.getKey().isAssignableFrom(h.type)) out.addAll(e.getValue());
        return new ArrayList<>(out);
    }

    private boolean listening(Class<? extends Event> type) {
        for (Handler h : handlers) if (h.type.isAssignableFrom(type)) return true;
        return false;
    }

    // ---- Aufrufen ----

    public void call(Event event) {
        if (event.isAsynchronous() && bridge.isMainThread() && !(event instanceof AsyncPlayerChatEvent)
                && !(event instanceof AsyncChatEvent) && !(event instanceof AsyncPlayerPreLoginEvent)
                && !(event instanceof ServerListPingEvent))
            LOG.fine(event.getEventName() + " ist asynchron, wird aber im Hauptthread aufgerufen");
        List<Handler> list;
        synchronized (this) {
            list = new ArrayList<>();
            for (Handler h : handlers) if (h.type.isInstance(event)) list.add(h);
        }
        list.sort((a, b) -> a.priority != b.priority ? Integer.compare(a.priority.getSlot(), b.priority.getSlot())
                : Long.compare(a.order, b.order));
        for (Handler h : list) {
            if (!h.plugin.isEnabled()) continue;
            if (h.ignoreCancelled && event instanceof Cancellable c && c.isCancelled()) continue;
            try {
                h.executor.execute(h.listener, event);
            } catch (Throwable t) {
                Throwable cause = t instanceof org.bukkit.event.EventException && t.getCause() != null ? t.getCause() : t;
                LOG.log(Level.SEVERE, "Could not pass event " + event.getEventName() + " to " + h.plugin.getDescription().getFullName(), cause);
            }
        }
    }

    // ---- CreeMC-Events übersetzen ----

    private static boolean cancelled(Event e) { return e instanceof Cancellable c && c.isCancelled(); }

    private static double[] xyz(Object o) {
        List<Object> l = Json.list(o);
        double[] out = new double[5];
        for (int i = 0; i < Math.min(5, l.size()); i++) out[i] = Json.num(l.get(i), 0);
        return out;
    }

    private Location loc(Object o) {
        double[] v = xyz(o);
        return new Location(bridge.server.world, v[0], v[1], v[2], (float) v[3], (float) v[4]);
    }

    private static List<Object> locList(Location l) { return List.of(l.getX(), l.getY(), l.getZ(), (double) l.getYaw(), (double) l.getPitch()); }

    private static DamageCause cause(String c) {
        return switch (c == null ? "" : c) {
            case "fall" -> DamageCause.FALL;
            case "attack" -> DamageCause.ENTITY_ATTACK;
            case "lava" -> DamageCause.LAVA;
            case "fire" -> DamageCause.FIRE_TICK;
            case "drown" -> DamageCause.DROWNING;
            case "void" -> DamageCause.VOID;
            case "starve" -> DamageCause.STARVATION;
            case "explosion" -> DamageCause.BLOCK_EXPLOSION;
            case "kill" -> DamageCause.KILL;
            default -> DamageCause.CUSTOM;
        };
    }

    private List<ItemStack> stacks(Object o) {
        List<ItemStack> out = new ArrayList<>();
        for (Object x : Json.list(o)) {
            List<Object> st = Json.list(x);
            if (st.size() >= 2) out.add(ItemStack.fromCree(Json.str(st.get(0), null), (int) Json.num(st.get(1), 1)));
        }
        return out;
    }

    private static List<Object> stackList(List<ItemStack> items) {
        List<Object> out = new ArrayList<>();
        for (ItemStack s : items) if (s != null && !s.isEmpty()) out.add(List.of(s.toCreeItem(), s.getAmount()));
        return out;
    }

    private ItemStack stack(Object o) {
        List<Object> st = Json.list(o);
        return st.size() >= 2 ? ItemStack.fromCree(Json.str(st.get(0), null), (int) Json.num(st.get(1), 1)) : null;
    }

    private static org.bukkit.event.inventory.InventoryAction action(int mode, int button) {
        return switch (mode) {
            case 0 -> button == 0 ? org.bukkit.event.inventory.InventoryAction.PICKUP_ALL : org.bukkit.event.inventory.InventoryAction.PICKUP_HALF;
            case 1 -> org.bukkit.event.inventory.InventoryAction.MOVE_TO_OTHER_INVENTORY;
            case 2 -> org.bukkit.event.inventory.InventoryAction.HOTBAR_SWAP;
            case 3 -> org.bukkit.event.inventory.InventoryAction.CLONE_STACK;
            case 4 -> button == 0 ? org.bukkit.event.inventory.InventoryAction.DROP_ONE_SLOT : org.bukkit.event.inventory.InventoryAction.DROP_ALL_SLOT;
            case 6 -> org.bukkit.event.inventory.InventoryAction.COLLECT_TO_CURSOR;
            default -> org.bukkit.event.inventory.InventoryAction.UNKNOWN;
        };
    }

    /** Fenster aus Server-Daten: eigenes GUI-Inventar oder eine Momentaufnahme (Kiste, Ofen ...). */
    private org.bukkit.inventory.InventoryView viewFor(PlayerImpl p, Map<String, Object> d) {
        GuiInventory g = GuiInventory.byId((int) Json.num(d.get("gui"), -1));
        if (g == null) {
            var type = org.bukkit.event.inventory.InventoryType.fromCree(Json.str(d.get("type"), "chest"));
            int size = (int) Json.num(d.get("size"), type.getDefaultSize());
            g = new GuiInventory(null, Math.max(1, size), Json.str(d.get("title"), type.getDefaultTitle()), type, false);
            List<Object> items = Json.list(d.get("items"));
            for (int i = 0; i < Math.min(items.size(), g.getSize()); i++) {
                ItemStack st = stack(items.get(i));
                if (st != null) g.setItem(i, st);   // id = -1: wird nicht an den Server geschickt
            }
        }
        return p.view(g);
    }

    Map<String, Object> fromServer(String name, Map<String, Object> d) {
        ServerImpl s = bridge.server;
        Map<String, Object> r = new LinkedHashMap<>();
        PlayerImpl p = d.get("player") instanceof Map<?, ?> snap ? s.updatePlayer(Json.obj(snap)) : null;
        switch (name) {
            case "player_join" -> {
                PlayerJoinEvent e = new PlayerJoinEvent(p, Json.str(d.get("join_message"), null));
                s.scoreboards().joined(p);   // Hauptscoreboard zeigen
                call(e);
                r.put("join_message", e.getJoinMessage());
            }
            case "player_quit" -> {
                PlayerQuitEvent e = new PlayerQuitEvent(p, Json.str(d.get("quit_message"), null));
                call(e);
                r.put("quit_message", e.getQuitMessage());
                s.scoreboards().quit(p);
                if (p != null) p.guiClosed();
                s.removePlayer(p);
            }
            case "player_chat" -> {
                String msg = Json.str(d.get("message"), "");
                String format = Json.str(d.get("format"), "<%1$s> %2$s");
                boolean cancel = false;
                if (listening(AsyncPlayerChatEvent.class)) {
                    AsyncPlayerChatEvent e = new AsyncPlayerChatEvent(true, p, msg, new LinkedHashSet<>(s.getOnlinePlayers()));
                    try {
                        e.setFormat(format);
                    } catch (Exception ignored) {
                    }
                    call(e);
                    cancel = e.isCancelled();
                    msg = e.getMessage();
                    format = e.getFormat();
                }
                if (listening(AsyncChatEvent.class)) {
                    AsyncChatEvent e = new AsyncChatEvent(true, p, new LinkedHashSet<Audience>(s.getOnlinePlayers()), Component.text(msg));
                    e.setCancelled(cancel);
                    call(e);
                    cancel = e.isCancelled();
                    msg = e.message().toLegacy().replaceAll("^§r", "");
                }
                r.put("cancelled", cancel);
                r.put("message", msg);
                r.put("format", format);
            }
            case "player_command" -> {
                PlayerCommandPreprocessEvent e = new PlayerCommandPreprocessEvent(p, "/" + Json.str(d.get("command"), ""));
                call(e);
                r.put("cancelled", e.isCancelled());
                r.put("command", e.getMessage().startsWith("/") ? e.getMessage().substring(1) : e.getMessage());
            }
            case "player_move" -> {
                Location to = loc(d.get("to"));
                PlayerMoveEvent e = new PlayerMoveEvent(p, loc(d.get("from")), to.clone());
                call(e);
                r.put("cancelled", e.isCancelled());
                if (e.getTo() != null && !e.getTo().equals(to)) r.put("to", locList(e.getTo()));
            }
            case "block_break" -> {
                BlockImpl b = s.world.block((int) Json.num(d.get("x"), 0), (int) Json.num(d.get("y"), 0), (int) Json.num(d.get("z"), 0),
                        Json.str(d.get("block"), null));
                BlockBreakEvent e = new BlockBreakEvent(b, p, true, 0);
                call(e);
                r.put("cancelled", e.isCancelled());
                r.put("drop_items", e.isDropItems());
            }
            case "block_place" -> {
                BlockImpl b = s.world.block((int) Json.num(d.get("x"), 0), (int) Json.num(d.get("y"), 0), (int) Json.num(d.get("z"), 0),
                        Json.str(d.get("block"), null));
                double[] a = xyz(d.get("against"));
                BlockPlaceEvent e = new BlockPlaceEvent(b, p, s.world.getBlockAt((int) a[0], (int) a[1], (int) a[2]),
                        ItemStack.fromCree(Json.str(d.get("item"), null), 1), true);
                call(e);
                r.put("cancelled", e.isCancelled() || !e.canBuild());
            }
            case "player_interact" -> {
                Block b = d.get("block") == null ? null : s.world.block((int) Json.num(d.get("x"), 0), (int) Json.num(d.get("y"), 0),
                        (int) Json.num(d.get("z"), 0), Json.str(d.get("block"), null));
                ItemStack item = d.get("item") == null ? null : ItemStack.fromCree(Json.str(d.get("item"), null), 1);
                PlayerInteractEvent e = new PlayerInteractEvent(p, b == null ? Action.RIGHT_CLICK_AIR : Action.RIGHT_CLICK_BLOCK,
                        item, b, d.get("face") == null ? BlockFace.SELF : BlockFace.fromProtocol((int) Json.num(d.get("face"), 0)),
                        Json.num(d.get("hand"), 0) == 1 ? EquipmentSlot.OFF_HAND : EquipmentSlot.HAND);
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_interact_entity" -> {
                PlayerInteractEntityEvent e = new PlayerInteractEntityEvent(p, s.entity(Json.obj(d.get("target"))));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_toggle_sneak" -> call(new PlayerToggleSneakEvent(p, Boolean.TRUE.equals(d.get("sneaking"))));
            case "player_toggle_sprint" -> call(new PlayerToggleSprintEvent(p, Boolean.TRUE.equals(d.get("sprinting"))));
            case "player_gamemode_change" -> {
                PlayerGameModeChangeEvent e = new PlayerGameModeChangeEvent(p, GameMode.getByValue((int) Json.num(d.get("new"), 0)));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_damage", "entity_damage" -> {
                Entity victim = p != null ? p : s.entity(Json.obj(d.get("entity")));
                Entity attacker = d.get("attacker") instanceof Map<?, ?> a ? s.entity(Json.obj(a)) : null;
                double amount = Json.num(d.get("amount"), 1);
                DamageCause dc = attacker != null && "generic".equals(d.get("cause")) ? DamageCause.ENTITY_ATTACK : cause(Json.str(d.get("cause"), null));
                EntityDamageEvent e = attacker != null ? new EntityDamageByEntityEvent(attacker, victim, dc, amount)
                        : new EntityDamageEvent(victim, dc, amount);
                call(e);
                r.put("cancelled", e.isCancelled());
                r.put("amount", e.getDamage());
            }
            case "player_death" -> {
                PlayerDeathEvent e = new PlayerDeathEvent(p, stacks(d.get("drops")), 0, Json.str(d.get("message"), null));
                e.setKeepInventory(Boolean.TRUE.equals(d.get("keep_inventory")));
                call(e);
                r.put("message", e.getDeathMessage());
                r.put("keep_inventory", e.getKeepInventory());
                r.put("drops", stackList(e.getDrops()));
            }
            case "entity_death" -> {
                EntityDeathEvent e = new EntityDeathEvent(s.entity(Json.obj(d.get("entity"))), stacks(d.get("drops")), 0);
                call(e);
                r.put("drops", stackList(e.getDrops()));
            }
            case "player_respawn" -> {
                PlayerRespawnEvent e = new PlayerRespawnEvent(p, loc(d.get("location")));
                call(e);
                if (e.getRespawnLocation() != null) r.put("location", locList(e.getRespawnLocation()));
            }
            case "player_pre_login" -> {
                java.net.InetAddress addr = null;
                try {
                    addr = java.net.InetAddress.getByName(Json.str(d.get("address"), "127.0.0.1"));
                } catch (Exception ignored) {
                }
                java.util.UUID id = d.get("uuid") == null ? null : java.util.UUID.fromString(Json.str(d.get("uuid"), null));
                AsyncPlayerPreLoginEvent e = new AsyncPlayerPreLoginEvent(Json.str(d.get("name"), ""), addr, id);
                call(e);
                r.put("cancelled", e.getLoginResult() != AsyncPlayerPreLoginEvent.Result.ALLOWED);
                r.put("kick_message", e.getKickMessage());
            }
            case "server_list_ping" -> {
                ServerListPingEvent e = new ServerListPingEvent(Json.str(d.get("address"), ""), Json.str(d.get("motd"), ""),
                        (int) Json.num(d.get("online"), 0), (int) Json.num(d.get("max_players"), 20));
                call(e);
                r.put("motd", e.getMotd());
                r.put("max_players", e.getMaxPlayers());
            }
            // ---- erweiterte API ----
            case "inventory_click" -> {
                var view = viewFor(p, d);
                int raw = (int) Json.num(d.get("slot"), -1);
                int mode = (int) Json.num(d.get("mode"), 0), button = (int) Json.num(d.get("button"), 0);
                var click = org.bukkit.event.inventory.ClickType.fromVanilla(mode, button, raw);
                var slotType = raw < 0 ? org.bukkit.event.inventory.InventoryType.SlotType.OUTSIDE
                        : raw < view.getTopInventory().getSize() ? org.bukkit.event.inventory.InventoryType.SlotType.CONTAINER
                        : raw >= view.getTopInventory().getSize() + 27 ? org.bukkit.event.inventory.InventoryType.SlotType.QUICKBAR
                        : org.bukkit.event.inventory.InventoryType.SlotType.CONTAINER;
                var e = new org.bukkit.event.inventory.InventoryClickEvent(view, slotType, raw, click, action(mode, button), mode == 2 ? button : -1);
                e.setCreeItems(stack(d.get("item")), stack(d.get("cursor")));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "inventory_update" -> {
                GuiInventory g = GuiInventory.byId((int) Json.num(d.get("gui"), -1));
                if (g != null) g.updateFromServer(Json.list(d.get("items")));
            }
            case "inventory_open" -> {
                var e = new org.bukkit.event.inventory.InventoryOpenEvent(viewFor(p, d));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "inventory_close" -> {
                var e = new org.bukkit.event.inventory.InventoryCloseEvent(viewFor(p, d));
                if (p != null) p.guiClosed();
                call(e);
            }
            case "player_drop_item" -> {
                ItemStack st = ItemStack.fromCree(Json.str(d.get("item"), null), (int) Json.num(d.get("count"), 1));
                var e = new org.bukkit.event.player.PlayerDropItemEvent(p, new ItemImpl(bridge, st, p.getLocation()));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_pickup_item" -> {
                ItemStack st = stack(d.get("item"));
                var e = new org.bukkit.event.entity.EntityPickupItemEvent(p, new ItemImpl(bridge, st, p.getLocation()), 0);
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_item_consume" -> {
                var e = new org.bukkit.event.player.PlayerItemConsumeEvent(p, ItemStack.fromCree(Json.str(d.get("item"), null), 1));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "food_level_change" -> {
                var e = new org.bukkit.event.entity.FoodLevelChangeEvent(p, (int) Json.num(d.get("food"), 20));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_item_held" -> {
                var e = new org.bukkit.event.player.PlayerItemHeldEvent(p, (int) Json.num(d.get("previous"), 0), (int) Json.num(d.get("new"), 0));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_swap_hand_items" -> {
                var e = new org.bukkit.event.player.PlayerSwapHandItemsEvent(p, p.getInventory().getItemInOffHand(), p.getInventory().getItemInMainHand());
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_toggle_flight" -> {
                var e = new org.bukkit.event.player.PlayerToggleFlightEvent(p, Boolean.TRUE.equals(d.get("flying")));
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "player_kick" -> {
                var e = new org.bukkit.event.player.PlayerKickEvent(p, Json.str(d.get("reason"), ""), null);
                call(e);
                r.put("cancelled", e.isCancelled());
                r.put("reason", e.getReason());
            }
            case "entity_explode" -> {
                Entity ent = s.entity(Json.obj(d.get("entity")));
                var e = new org.bukkit.event.entity.EntityExplodeEvent(ent, ent.getLocation(), new ArrayList<>(), 1f);
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            case "creature_spawn" -> {
                Entity ent = s.entity(Json.obj(d.get("entity")));
                var e = new org.bukkit.event.entity.CreatureSpawnEvent(ent, org.bukkit.event.entity.CreatureSpawnEvent.SpawnReason.NATURAL);
                call(e);
                r.put("cancelled", e.isCancelled());
            }
            default -> {
            }
        }
        return r;
    }

    /** Für Logs/Tests: wer hört auf was. */
    public synchronized List<String> describeHandlers() {
        List<String> out = new ArrayList<>();
        for (Handler h : handlers) out.add(h.plugin.getName() + ":" + h.type.getSimpleName());
        return out;
    }

    static boolean isCancelled(Event e) { return cancelled(e); }

    static Player asPlayer(Object o) { return o instanceof Player pl ? pl : null; }
}
