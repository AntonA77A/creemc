package org.bukkit.event.entity;

import java.util.List;
import net.kyori.adventure.text.Component;
import org.bukkit.entity.Player;
import org.bukkit.event.HandlerList;
import org.bukkit.inventory.ItemStack;

public class PlayerDeathEvent extends EntityDeathEvent {
    private static final HandlerList HANDLERS = new HandlerList();
    private String deathMessage;
    private boolean keepInventory;
    private boolean keepLevel;

    public PlayerDeathEvent(Player player, List<ItemStack> drops, int droppedExp, String deathMessage) {
        super(player, drops, droppedExp);
        this.deathMessage = deathMessage;
    }

    @Override
    public Player getEntity() { return (Player) entity; }

    public Player getPlayer() { return (Player) entity; }

    public String getDeathMessage() { return deathMessage; }

    public void setDeathMessage(String deathMessage) { this.deathMessage = deathMessage; }

    public Component deathMessage() { return deathMessage == null ? null : Component.text(deathMessage); }

    public void deathMessage(Component message) { deathMessage = message == null ? null : message.toLegacy(); }

    public boolean getKeepInventory() { return keepInventory; }

    public void setKeepInventory(boolean keep) { keepInventory = keep; }

    public boolean getKeepLevel() { return keepLevel; }

    public void setKeepLevel(boolean keep) { keepLevel = keep; }

    @Override
    public HandlerList getHandlers() { return HANDLERS; }

    public static HandlerList getHandlerList() { return HANDLERS; }
}
