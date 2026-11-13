package creemc.bridge;

import java.util.UUID;
import org.bukkit.OfflinePlayer;
import org.bukkit.entity.Player;

final class OfflineImpl implements OfflinePlayer {
    private final Bridge bridge;
    private final UUID id;
    private final String name;

    OfflineImpl(Bridge bridge, UUID id, String name) {
        this.bridge = bridge;
        this.id = id;
        this.name = name;
    }

    @Override public boolean isOnline() { return getPlayer() != null; }

    @Override public String getName() { return name; }

    @Override public UUID getUniqueId() { return id; }

    @Override public Player getPlayer() { return bridge.server.getPlayer(id); }

    @Override public boolean hasPlayedBefore() { return Boolean.TRUE.equals(bridge.request("played_before", "uuid", id.toString())); }

    @Override public boolean isOp() { return name != null && Boolean.TRUE.equals(bridge.request("is_op", "name", name)); }

    @Override
    public void setOp(boolean value) {
        if (name != null) bridge.call("op", "name", name, "value", value);
    }

    @Override public boolean equals(Object o) { return o instanceof OfflinePlayer p && p.getUniqueId().equals(id); }

    @Override public int hashCode() { return id.hashCode(); }
}
