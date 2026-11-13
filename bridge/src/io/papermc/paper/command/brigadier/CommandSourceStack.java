package io.papermc.paper.command.brigadier;

import org.bukkit.Location;
import org.bukkit.command.CommandSender;
import org.bukkit.entity.Entity;

public interface CommandSourceStack {
    Location getLocation();

    CommandSender getSender();

    Entity getExecutor();
}
