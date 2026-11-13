package creemc.bridge;

import io.papermc.paper.command.brigadier.BasicCommand;
import io.papermc.paper.command.brigadier.CommandSourceStack;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.bukkit.Location;
import org.bukkit.command.Command;
import org.bukkit.command.CommandSender;
import org.bukkit.command.PluginCommand;
import org.bukkit.entity.Entity;
import org.bukkit.plugin.Plugin;

/** Befehle der Java-Plugins (plugin.yml und Paper-BasicCommand). */
public final class Commands {
    private final Bridge bridge;
    private final Map<String, Command> byLabel = new LinkedHashMap<>();   // Name, Aliase, plugin:name
    private final List<Command> all = new ArrayList<>();
    boolean announced;

    Commands(Bridge bridge) { this.bridge = bridge; }

    private static final class Basic extends Command {
        final Plugin plugin;
        final BasicCommand cmd;

        Basic(Plugin plugin, String label, String description, Collection<String> aliases, BasicCommand cmd) {
            super(label, description, "/" + label, new ArrayList<>(aliases));
            this.plugin = plugin;
            this.cmd = cmd;
            setPermission(cmd.permission());
        }

        @Override
        public boolean execute(CommandSender sender, String label, String[] args) {
            if (!cmd.canUse(sender)) {
                sender.sendMessage("§cI'm sorry, but you do not have permission to perform this command.");
                return true;
            }
            cmd.execute(source(sender), args);
            return true;
        }

        @Override
        public List<String> tabComplete(CommandSender sender, String alias, String[] args) {
            return new ArrayList<>(cmd.suggest(source(sender), args));
        }

        private static CommandSourceStack source(CommandSender sender) {
            return new CommandSourceStack() {
                @Override
                public Location getLocation() {
                    return sender instanceof Entity e ? e.getLocation() : org.bukkit.Bukkit.getWorlds().get(0).getSpawnLocation();
                }

                @Override
                public CommandSender getSender() { return sender; }

                @Override
                public Entity getExecutor() { return sender instanceof Entity e ? e : null; }
            };
        }
    }

    private void add(Plugin plugin, Command c) {
        all.add(c);
        String prefix = plugin.getName().toLowerCase() + ":";
        for (String label : labels(c)) {
            byLabel.putIfAbsent(label, c);
            byLabel.put(prefix + label, c);
        }
        if (announced) bridge.call("commands", "list", describe());
    }

    private static List<String> labels(Command c) {
        List<String> l = new ArrayList<>();
        l.add(c.getName().toLowerCase());
        for (String a : c.getAliases()) l.add(a.toLowerCase());
        return l;
    }

    @SuppressWarnings("unchecked")
    void registerFromDescription(Plugin plugin) {
        for (Map.Entry<String, Map<String, Object>> e : plugin.getDescription().getCommands().entrySet()) {
            PluginCommand c = new PluginCommand(e.getKey(), plugin);
            Map<String, Object> m = e.getValue();
            if (m.get("description") != null) c.setDescription(String.valueOf(m.get("description")));
            if (m.get("usage") != null) c.setUsage(String.valueOf(m.get("usage")));
            if (m.get("permission") != null) c.setPermission(String.valueOf(m.get("permission")));
            if (m.get("permission-message") != null) c.setPermissionMessage(String.valueOf(m.get("permission-message")));
            Object aliases = m.get("aliases");
            if (aliases instanceof List<?> l) c.setAliases(l.stream().map(String::valueOf).toList());
            else if (aliases != null) c.setAliases(List.of(String.valueOf(aliases)));
            add(plugin, c);
        }
    }

    public PluginCommand pluginCommand(Plugin plugin, String name) {
        Command c = byLabel.get(name.toLowerCase());
        if (c == null) c = byLabel.get(plugin.getName().toLowerCase() + ":" + name.toLowerCase());
        return c instanceof PluginCommand pc && pc.getPlugin() == plugin ? pc : null;
    }

    public PluginCommand findPluginCommand(String name) {
        return byLabel.get(name.toLowerCase()) instanceof PluginCommand pc ? pc : null;
    }

    public void registerBasic(Plugin plugin, String label, String description, Collection<String> aliases, BasicCommand cmd) {
        add(plugin, new Basic(plugin, label, description == null ? "" : description, aliases, cmd));
    }

    public List<Map<String, Object>> describe() {
        announced = true;
        List<Map<String, Object>> out = new ArrayList<>();
        for (Command c : all) {
            Map<String, Object> m = new LinkedHashMap<>();
            m.put("name", c.getName());
            m.put("aliases", c.getAliases());
            m.put("description", c.getDescription());
            m.put("usage", c.getUsage());
            out.add(m);
        }
        return out;
    }

    public boolean has(String label) { return byLabel.containsKey(label.toLowerCase()); }

    /** Führt einen Befehl aus; true, wenn ein Java-Plugin ihn kennt. */
    public boolean dispatch(CommandSender sender, String line) {
        String[] parts = line.startsWith("/") ? line.substring(1).split(" ", -1) : line.split(" ", -1);
        Command c = byLabel.get(parts[0].toLowerCase());
        if (c == null) return false;
        String[] args = java.util.Arrays.copyOfRange(parts, 1, parts.length);
        try {
            c.execute(sender, parts[0], args);
        } catch (Throwable t) {
            Logger.getLogger("CreeMC-Bridge").log(Level.SEVERE, "Fehler im Befehl /" + parts[0], t);
            sender.sendMessage("§cAn internal error occurred while attempting to perform this command");
        }
        return true;
    }

    Map<String, Object> execute(Map<String, Object> msg) {
        CommandSender sender = bridge.server.senderFor(Json.str(msg.get("sender"), null));
        StringBuilder line = new StringBuilder(Json.str(msg.get("label"), ""));
        for (Object a : Json.list(msg.get("args"))) line.append(' ').append(a);
        return Map.of("ok", dispatch(sender, line.toString()));
    }

    List<String> complete(Map<String, Object> msg) {
        Command c = byLabel.get(Json.str(msg.get("label"), "").toLowerCase());
        if (c == null) return List.of();
        CommandSender sender = bridge.server.senderFor(Json.str(msg.get("sender"), null));
        List<Object> raw = Json.list(msg.get("args"));
        String[] args = raw.stream().map(String::valueOf).toArray(String[]::new);
        try {
            return c.tabComplete(sender, c.getName(), args);
        } catch (Throwable t) {
            return List.of();
        }
    }
}
