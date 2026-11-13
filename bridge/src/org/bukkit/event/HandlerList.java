package org.bukkit.event;

import java.util.ArrayList;
import java.util.List;
import org.bukkit.plugin.Plugin;

/** Nur für Kompatibilität: CreeMC verteilt Events zentral (creemc.bridge.Events). */
public class HandlerList {
    private static final List<HandlerList> ALL = new ArrayList<>();

    public HandlerList() {
        synchronized (ALL) {
            ALL.add(this);
        }
    }

    public static void unregisterAll(Plugin plugin) { creemc.bridge.Events.unregister(plugin); }

    public static void unregisterAll(Listener listener) { creemc.bridge.Events.unregister(listener); }

    public void unregister(Plugin plugin) { creemc.bridge.Events.unregister(plugin); }

    public void unregister(Listener listener) { creemc.bridge.Events.unregister(listener); }

    public void unregisterAll() { }
}
