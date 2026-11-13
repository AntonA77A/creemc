package org.bukkit.event.server;

import net.kyori.adventure.text.Component;
import org.bukkit.event.Event;
import org.bukkit.event.HandlerList;

public class ServerListPingEvent extends Event {
    private static final HandlerList HANDLERS = new HandlerList();
    private final String hostname;
    private String motd;
    private final int numPlayers;
    private int maxPlayers;

    public ServerListPingEvent(String hostname, String motd, int numPlayers, int maxPlayers) {
        super(true);
        this.hostname = hostname;
        this.motd = motd;
        this.numPlayers = numPlayers;
        this.maxPlayers = maxPlayers;
    }

    public String getHostname() { return hostname; }

    public String getMotd() { return motd; }

    public void setMotd(String motd) { this.motd = motd; }

    public Component motd() { return Component.text(motd); }

    public void motd(Component motd) { this.motd = motd.toLegacy(); }

    public int getNumPlayers() { return numPlayers; }

    public int getMaxPlayers() { return maxPlayers; }

    public void setMaxPlayers(int maxPlayers) { this.maxPlayers = maxPlayers; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
