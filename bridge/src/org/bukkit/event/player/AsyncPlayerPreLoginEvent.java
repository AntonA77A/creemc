package org.bukkit.event.player;

import java.net.InetAddress;
import java.util.UUID;
import net.kyori.adventure.text.Component;
import org.bukkit.event.Event;
import org.bukkit.event.HandlerList;

public class AsyncPlayerPreLoginEvent extends Event {
    private static final HandlerList HANDLERS = new HandlerList();

    public enum Result { ALLOWED, KICK_FULL, KICK_BANNED, KICK_WHITELIST, KICK_OTHER }

    private final String name;
    private final UUID uniqueId;
    private final InetAddress address;
    private Result result = Result.ALLOWED;
    private String message = "";

    public AsyncPlayerPreLoginEvent(String name, InetAddress address, UUID uniqueId) {
        super(true);
        this.name = name;
        this.address = address;
        this.uniqueId = uniqueId;
    }

    public String getName() { return name; }

    public UUID getUniqueId() { return uniqueId; }

    public InetAddress getAddress() { return address; }

    public Result getLoginResult() { return result; }

    public void setLoginResult(Result result) { this.result = result; }

    public String getKickMessage() { return message; }

    public void setKickMessage(String message) { this.message = message; }

    public void kickMessage(Component message) { this.message = message.toLegacy(); }

    public void allow() {
        result = Result.ALLOWED;
        message = "";
    }

    public void disallow(Result result, String message) {
        this.result = result;
        this.message = message;
    }

    public void disallow(Result result, Component message) { disallow(result, message.toLegacy()); }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
