package org.bukkit.command;

import java.util.ArrayList;
import java.util.List;
import org.bukkit.plugin.Plugin;

public final class PluginCommand extends Command implements PluginIdentifiableCommand {
    private final Plugin owningPlugin;
    private CommandExecutor executor;
    private TabCompleter completer;

    public PluginCommand(String name, Plugin owner) {
        super(name);
        this.owningPlugin = owner;
        this.usageMessage = "";
    }

    @Override
    public boolean execute(CommandSender sender, String commandLabel, String[] args) {
        if (!owningPlugin.isEnabled() || !testPermission(sender)) return true;
        CommandExecutor ex = executor != null ? executor : owningPlugin;
        boolean ok;
        try {
            ok = ex.onCommand(sender, this, commandLabel, args);
        } catch (Throwable t) {
            throw new CommandException("Unhandled exception executing command '" + commandLabel + "' in plugin "
                    + owningPlugin.getName(), t);
        }
        if (!ok && usageMessage != null && !usageMessage.isEmpty()) {
            for (String line : usageMessage.replace("<command>", commandLabel).split("\n")) sender.sendMessage(line);
        }
        return ok;
    }

    @Override
    public List<String> tabComplete(CommandSender sender, String alias, String[] args) {
        TabCompleter c = completer != null ? completer : executor instanceof TabCompleter t ? t : owningPlugin;
        List<String> r = c == null ? null : c.onTabComplete(sender, this, alias, args);
        return r == null ? new ArrayList<>() : r;
    }

    public void setExecutor(CommandExecutor executor) { this.executor = executor; }

    public CommandExecutor getExecutor() { return executor != null ? executor : owningPlugin; }

    public void setTabCompleter(TabCompleter completer) { this.completer = completer; }

    public TabCompleter getTabCompleter() { return completer; }

    @Override
    public Plugin getPlugin() { return owningPlugin; }
}
