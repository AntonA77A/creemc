package org.bukkit.entity;

import java.util.UUID;
import org.bukkit.Location;
import org.bukkit.World;
import org.bukkit.command.CommandSender;
import org.bukkit.util.Vector;

public interface Entity extends CommandSender {
    Location getLocation();

    boolean teleport(Location location);

    boolean teleport(Entity destination);

    World getWorld();

    UUID getUniqueId();

    int getEntityId();

    EntityType getType();

    boolean isDead();

    boolean isValid();

    void remove();

    boolean isOnGround();

    Vector getVelocity();

    void setVelocity(Vector velocity);

    int getFireTicks();

    void setFireTicks(int ticks);
}
