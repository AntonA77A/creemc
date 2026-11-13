package org.bukkit.entity;

import org.bukkit.Location;

public interface LivingEntity extends Damageable {
    Location getEyeLocation();

    double getEyeHeight();

    int getRemainingAir();

    void setRemainingAir(int ticks);

    int getMaximumAir();
}
