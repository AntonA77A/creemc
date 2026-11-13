package org.bukkit.command;

import java.util.ArrayList;
import java.util.List;

public abstract class Command {
    private String name;
    private String label;
    private List<String> aliases = new ArrayList<>();
    protected String description = "";
    protected String usageMessage;
    private String permission;
    private String permissionMessage;

    protected Command(String name) { this(name, "", "/" + name, new ArrayList<>()); }

    protected Command(String name, String description, String usageMessage, List<String> aliases) {
        this.name = name;
        this.label = name;
        this.description = description == null ? "" : description;
        this.usageMessage = usageMessage == null ? "/" + name : usageMessage;
        this.aliases = new ArrayList<>(aliases);
    }

    public abstract boolean execute(CommandSender sender, String commandLabel, String[] args);

    public List<String> tabComplete(CommandSender sender, String alias, String[] args) { return new ArrayList<>(); }

    public String getName() { return name; }

    public boolean setName(String name) { this.name = name; return true; }

    public String getLabel() { return label; }

    public boolean setLabel(String label) { this.label = label; return true; }

    public List<String> getAliases() { return aliases; }

    public Command setAliases(List<String> aliases) { this.aliases = new ArrayList<>(aliases); return this; }

    public String getDescription() { return description; }

    public Command setDescription(String description) { this.description = description == null ? "" : description; return this; }

    public String getUsage() { return usageMessage; }

    public Command setUsage(String usage) { this.usageMessage = usage == null ? "" : usage; return this; }

    public String getPermission() { return permission; }

    public void setPermission(String permission) { this.permission = permission; }

    public String getPermissionMessage() { return permissionMessage; }

    public Command setPermissionMessage(String message) { this.permissionMessage = message; return this; }

    public boolean testPermissionSilent(CommandSender target) {
        if (permission == null || permission.isEmpty()) return true;
        for (String p : permission.split(";")) if (target.hasPermission(p)) return true;
        return false;
    }

    public boolean testPermission(CommandSender target) {
        if (testPermissionSilent(target)) return true;
        target.sendMessage(permissionMessage != null && !permissionMessage.isEmpty() ? permissionMessage
                : "§cI'm sorry, but you do not have permission to perform this command.");
        return false;
    }

    @Override
    public String toString() { return getClass().getName() + "(" + name + ")"; }
}
