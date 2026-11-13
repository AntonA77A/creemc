package io.papermc.paper.event.player;

import java.util.Set;
import net.kyori.adventure.audience.Audience;
import net.kyori.adventure.text.Component;
import org.bukkit.entity.Player;
import org.bukkit.event.Cancellable;
import org.bukkit.event.HandlerList;
import org.bukkit.event.player.PlayerEvent;

/** Paper-Chat-Event (vereinfacht: ohne Renderer). */
public class AsyncChatEvent extends PlayerEvent implements Cancellable {
    private static final HandlerList HANDLERS = new HandlerList();
    private boolean cancelled;
    private Component message;
    private final Component original;
    private final Set<Audience> viewers;

    public AsyncChatEvent(boolean async, Player player, Set<Audience> viewers, Component message) {
        super(player, async);
        this.viewers = viewers;
        this.message = message;
        this.original = message;
    }

    public Component message() { return message; }

    public void message(Component message) { this.message = message; }

    public Component originalMessage() { return original; }

    public Set<Audience> viewers() { return viewers; }

    @Override
    public boolean isCancelled() { return cancelled; }

    @Override
    public void setCancelled(boolean cancel) { cancelled = cancel; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
