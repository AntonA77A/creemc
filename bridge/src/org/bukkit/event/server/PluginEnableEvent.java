package org.bukkit.event.server;

import org.bukkit.event.Event;
import org.bukkit.event.HandlerList;
import org.bukkit.plugin.Plugin;

public class PluginEnableEvent extends Event {
    private static final HandlerList HANDLERS = new HandlerList();
    private final Plugin plugin;

    public PluginEnableEvent(Plugin plugin) { this.plugin = plugin; }

    public Plugin getPlugin() { return plugin; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
