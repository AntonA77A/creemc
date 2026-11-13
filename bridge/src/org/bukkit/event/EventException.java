package org.bukkit.event;

public class EventException extends Exception {
    public EventException(Throwable cause) { super(cause); }

    public EventException(Throwable cause, String message) { super(message, cause); }

    public EventException(String message) { super(message); }
}
