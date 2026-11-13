package org.bukkit.event.player;

import org.bukkit.Location;
import org.bukkit.entity.Player;
import org.bukkit.event.HandlerList;

public class PlayerRespawnEvent extends PlayerEvent {
    private static final HandlerList HANDLERS = new HandlerList();
    private Location respawnLocation;

    public PlayerRespawnEvent(Player who, Location respawnLocation) {
        super(who);
        this.respawnLocation = respawnLocation;
    }

    public Location getRespawnLocation() { return respawnLocation; }

    public void setRespawnLocation(Location respawnLocation) { this.respawnLocation = respawnLocation; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
