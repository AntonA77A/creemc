package creemc.bridge;

import java.net.InetSocketAddress;
import java.util.Map;
import java.util.UUID;
import org.bukkit.GameMode;
import org.bukkit.Location;
import org.bukkit.entity.EntityType;
import org.bukkit.entity.Player;
import org.bukkit.inventory.ItemStack;
import org.bukkit.inventory.PlayerInventory;
import org.bukkit.util.Vector;

/** Spieler: liest aus dem Schnappschuss des Servers, Änderungen gehen als Auftrag an CreeMC. */
final class PlayerImpl extends EntityImpl implements Player {
    String name = "";
    String displayName;
    String listName;
    int food = 20, gamemode, level, totalXp, heldSlot, ping, air = 300;
    float saturation = 5, exp;
    boolean op, sneaking, sprinting, flying, allowFlight;
    volatile boolean online = true;
    String handItem;
    int handCount;
    String ip = "127.0.0.1", locale = "en_us", brand = "vanilla";
    int port;
    private final InventoryImpl inventory;

    PlayerImpl(Bridge bridge, UUID uuid) {
        super(bridge, -1);
        this.uuid = uuid;
        this.type = "player";
        this.inventory = new InventoryImpl(bridge, this);
    }

    @Override
    void update(Map<String, Object> s) {
        super.update(s);
        name = Json.str(s.get("name"), name);
        if (s.containsKey("display")) displayName = Json.str(s.get("display"), null);
        food = (int) Json.num(s.get("food"), food);
        saturation = (float) Json.num(s.get("sat"), saturation);
        gamemode = (int) Json.num(s.get("gm"), gamemode);
        op = Boolean.TRUE.equals(s.getOrDefault("op", op));
        level = (int) Json.num(s.get("level"), level);
        exp = (float) Json.num(s.get("exp"), exp);
        totalXp = (int) Json.num(s.get("xpt"), totalXp);
        sneaking = Boolean.TRUE.equals(s.getOrDefault("sneak", sneaking));
        sprinting = Boolean.TRUE.equals(s.getOrDefault("sprint", sprinting));
        flying = Boolean.TRUE.equals(s.getOrDefault("fly", flying));
        heldSlot = (int) Json.num(s.get("slot"), heldSlot);
        if (s.containsKey("hand")) {
            handItem = Json.str(s.get("hand"), null);
            handCount = (int) Json.num(s.get("hand_n"), 1);
        }
        ip = Json.str(s.get("ip"), ip);
        port = (int) Json.num(s.get("port"), port);
        locale = Json.str(s.get("locale"), locale);
        ping = (int) Json.num(s.get("ping"), ping);
        air = (int) Json.num(s.get("air"), air);
        if (s.get("id") != null) idOverride = (int) Json.num(s.get("id"), -1);
    }

    private int idOverride = -1;

    private String me() { return uuid.toString(); }

    // ---- Entity ----

    @Override public int getEntityId() { return idOverride; }

    @Override public EntityType getType() { return EntityType.PLAYER; }

    @Override public boolean isValid() { return online && !dead; }

    @Override
    public boolean teleport(Location l) {
        bridge.call("teleport", "to", me(), "x", l.getX(), "y", l.getY(), "z", l.getZ(), "yaw", l.getYaw(), "pitch", l.getPitch());
        x = l.getX();
        y = l.getY();
        z = l.getZ();
        yaw = l.getYaw();
        pitch = l.getPitch();
        return true;
    }

    @Override public void remove() { kickPlayer(""); }

    @Override public void setVelocity(Vector v) { bridge.call("velocity", "to", me(), "x", v.getX(), "y", v.getY(), "z", v.getZ()); }

    @Override
    public void setFireTicks(int ticks) {
        fireTicks = ticks;
        bridge.call("fire", "to", me(), "ticks", ticks);
    }

    @Override
    public void setHealth(double h) {
        if (h < 0 || h > getMaxHealth()) throw new IllegalArgumentException("Health must be between 0 and " + getMaxHealth() + "(" + h + ")");
        health = h;
        bridge.call("health", "to", me(), "value", h);
    }

    @Override public void damage(double amount) { bridge.call("damage", "to", me(), "amount", amount); }

    @Override
    public void damage(double amount, org.bukkit.entity.Entity source) {
        bridge.call("damage", "to", me(), "amount", amount, "source", source == null ? null : source.getUniqueId().toString());
    }

    @Override public double getEyeHeight() { return sneaking ? 1.27 : 1.62; }

    @Override public int getRemainingAir() { return air; }

    @Override
    public void setRemainingAir(int ticks) {
        air = ticks;
        bridge.call("air", "to", me(), "value", ticks);
    }

    // ---- CommandSender ----

    @Override
    public void sendMessage(String message) {
        if (message != null) bridge.call("msg", "to", me(), "text", message);
    }

    @Override
    public void sendMessage(String... messages) {
        for (String m : messages) sendMessage(m);
    }

    @Override public String getName() { return name; }

    @Override public boolean hasPermission(String perm) { return Perms.has(op, perm); }

    @Override public boolean isPermissionSet(String perm) { return Perms.isSet(perm); }

    @Override public boolean isOp() { return op; }

    @Override
    public void setOp(boolean value) {
        op = value;
        bridge.call("op", "name", name, "value", value);
    }

    // ---- HumanEntity ----

    @Override public PlayerInventory getInventory() { return inventory; }

    @Override public GameMode getGameMode() { return GameMode.getByValue(gamemode); }

    @Override
    public void setGameMode(GameMode mode) {
        gamemode = mode.getValue();
        bridge.call("gamemode", "to", me(), "mode", mode.getValue());
    }

    @Override public ItemStack getItemInHand() { return inventory.getItemInMainHand(); }

    @Override public void closeInventory() { bridge.call("close_window", "to", me()); }

    // ---- OfflinePlayer ----

    @Override public boolean isOnline() { return online; }

    @Override public Player getPlayer() { return online ? this : null; }

    @Override public boolean hasPlayedBefore() { return true; }

    // ---- Player ----

    @Override public String getDisplayName() { return displayName != null ? displayName : name; }

    @Override
    public void setDisplayName(String n) {
        displayName = n;
        bridge.call("display_name", "to", me(), "name", n);
    }

    @Override public String getPlayerListName() { return listName != null ? listName : name; }

    @Override
    public void setPlayerListName(String n) {
        listName = n;
        bridge.call("list_name", "to", me(), "name", n);
    }

    @Override public void kickPlayer(String message) { bridge.call("kick", "to", me(), "reason", message == null ? "" : message); }

    @Override public void chat(String message) { bridge.call("chat", "to", me(), "text", message); }

    @Override
    public boolean performCommand(String command) {
        if (bridge.commands.dispatch(this, command)) return true;
        bridge.call("command", "sender", me(), "line", command);
        return true;
    }

    @Override public boolean isSneaking() { return sneaking; }

    @Override public boolean isSprinting() { return sprinting; }

    @Override public boolean isFlying() { return flying; }

    @Override public int getFoodLevel() { return food; }

    @Override
    public void setFoodLevel(int value) {
        food = value;
        bridge.call("food", "to", me(), "value", value, "sat", saturation);
    }

    @Override public float getSaturation() { return saturation; }

    @Override
    public void setSaturation(float value) {
        saturation = value;
        bridge.call("food", "to", me(), "value", food, "sat", value);
    }

    @Override public int getLevel() { return level; }

    @Override
    public void setLevel(int l) {
        level = l;
        bridge.call("level", "to", me(), "value", l);
    }

    @Override public float getExp() { return exp; }

    @Override
    public void setExp(float e) {
        exp = e;
        bridge.call("exp", "to", me(), "value", e);
    }

    @Override public int getTotalExperience() { return totalXp; }

    @Override
    public void setTotalExperience(int value) {
        totalXp = value;
        bridge.call("total_xp", "to", me(), "value", value);
    }

    @Override public void giveExp(int amount) { bridge.call("give_xp", "to", me(), "value", amount); }

    @Override public void giveExpLevels(int amount) { setLevel(Math.max(0, level + amount)); }

    @Override public void sendTitle(String title, String subtitle) { sendTitle(title, subtitle, 10, 70, 20); }

    @Override
    public void sendTitle(String title, String subtitle, int fadeIn, int stay, int fadeOut) {
        bridge.call("title", "to", me(), "title", title == null ? "" : title, "sub", subtitle == null ? "" : subtitle,
                "fade_in", fadeIn, "stay", stay, "fade_out", fadeOut);
    }

    @Override public void sendActionBar(String message) { bridge.call("actionbar", "to", me(), "text", message); }

    @Override
    public void setPlayerListHeaderFooter(String header, String footer) {
        bridge.call("tablist", "to", me(), "header", header == null ? "" : header, "footer", footer == null ? "" : footer);
    }

    @Override
    public void playSound(Location l, String sound, float volume, float pitch) {
        bridge.call("sound", "to", me(), "name", sound.contains(":") ? sound : "minecraft:" + sound, "volume", volume, "pitch", pitch);
    }

    @Override public InetSocketAddress getAddress() { return new InetSocketAddress(ip, port); }

    @Override public int getPing() { return ping; }

    @Override public String getLocale() { return locale; }

    @Override public void updateInventory() { bridge.call("sync_inventory", "to", me()); }

    @Override public Location getCompassTarget() { return bridge.server.world.getSpawnLocation(); }

    @Override public Location getBedSpawnLocation() { return null; }

    @Override public boolean getAllowFlight() { return allowFlight || gamemode == 1 || gamemode == 3; }

    @Override
    public void setAllowFlight(boolean flight) {
        allowFlight = flight;
        bridge.call("allow_flight", "to", me(), "value", flight);
    }

    @Override public void setFlying(boolean value) { flying = value; }

    @Override public String getClientBrandName() { return brand; }

    @Override public boolean equals(Object o) { return o instanceof Player p && p.getUniqueId().equals(uuid); }

    @Override public int hashCode() { return uuid.hashCode(); }

    @Override public String toString() { return "CraftPlayer{name=" + name + "}"; }
}
