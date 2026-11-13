package org.bukkit.permissions;

public interface Permissible extends ServerOperator {
    boolean hasPermission(String name);

    boolean isPermissionSet(String name);
}
