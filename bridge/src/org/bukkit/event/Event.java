package org.bukkit.event;

public abstract class Event {
    public enum Result { DENY, DEFAULT, ALLOW }

    private final boolean async;
    private String name;

    public Event() { this(false); }

    public Event(boolean isAsync) { this.async = isAsync; }

    public String getEventName() {
        if (name == null) name = getClass().getSimpleName();
        return name;
    }

    public abstract HandlerList getHandlers();

    public final boolean isAsynchronous() { return async; }

    public boolean callEvent() {
        org.bukkit.Bukkit.getPluginManager().callEvent(this);
        return !(this instanceof Cancellable c) || !c.isCancelled();
    }
}
