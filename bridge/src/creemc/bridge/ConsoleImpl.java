package creemc.bridge;

import org.bukkit.Bukkit;
import org.bukkit.Server;
import org.bukkit.command.ConsoleCommandSender;

final class ConsoleImpl implements ConsoleCommandSender {
    private final Bridge bridge;

    ConsoleImpl(Bridge bridge) { this.bridge = bridge; }

    @Override public void sendMessage(String message) { bridge.call("msg", "to", null, "text", message); }

    @Override
    public void sendMessage(String... messages) {
        for (String m : messages) sendMessage(m);
    }

    @Override public Server getServer() { return Bukkit.getServer(); }

    @Override public String getName() { return "CONSOLE"; }

    @Override public boolean hasPermission(String name) { return true; }

    @Override public boolean isPermissionSet(String name) { return true; }

    @Override public boolean isOp() { return true; }

    @Override public void setOp(boolean value) { }
}
