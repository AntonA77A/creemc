package org.bukkit.entity;

public interface Damageable extends Entity {
    double getHealth();

    void setHealth(double health);

    @Deprecated
    double getMaxHealth();

    void damage(double amount);

    void damage(double amount, Entity source);
}
