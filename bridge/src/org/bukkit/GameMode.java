package org.bukkit;

public enum GameMode {
    SURVIVAL(0), CREATIVE(1), ADVENTURE(2), SPECTATOR(3);

    private final int value;

    GameMode(int value) { this.value = value; }

    @Deprecated
    public int getValue() { return value; }

    @Deprecated
    public static GameMode getByValue(int value) {
        for (GameMode m : values()) if (m.value == value) return m;
        return null;
    }
}
