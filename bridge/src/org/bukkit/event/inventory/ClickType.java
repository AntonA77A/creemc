package org.bukkit.event.inventory;

public enum ClickType {
    LEFT, SHIFT_LEFT, RIGHT, SHIFT_RIGHT, WINDOW_BORDER_LEFT, WINDOW_BORDER_RIGHT, MIDDLE, NUMBER_KEY, DOUBLE_CLICK, DROP,
    CONTROL_DROP, CREATIVE, SWAP_OFFHAND, UNKNOWN;

    public boolean isKeyboardClick() { return this == NUMBER_KEY || this == DROP || this == CONTROL_DROP || this == SWAP_OFFHAND; }

    public boolean isCreativeAction() { return this == MIDDLE || this == CREATIVE; }

    public boolean isRightClick() { return this == RIGHT || this == SHIFT_RIGHT; }

    public boolean isLeftClick() { return this == LEFT || this == SHIFT_LEFT || this == DOUBLE_CLICK || this == CREATIVE; }

    public boolean isShiftClick() { return this == SHIFT_LEFT || this == SHIFT_RIGHT; }

    /** CreeMC: Klick-Art aus dem Vanilla-Paket (Modus, Taste). */
    public static ClickType fromVanilla(int mode, int button, int slot) {
        return switch (mode) {
            case 0 -> slot == -999 ? (button == 0 ? WINDOW_BORDER_LEFT : WINDOW_BORDER_RIGHT) : (button == 0 ? LEFT : RIGHT);
            case 1 -> button == 0 ? SHIFT_LEFT : SHIFT_RIGHT;
            case 2 -> button == 40 ? SWAP_OFFHAND : NUMBER_KEY;
            case 3 -> MIDDLE;
            case 4 -> slot == -999 ? (button == 0 ? WINDOW_BORDER_LEFT : WINDOW_BORDER_RIGHT) : (button == 0 ? DROP : CONTROL_DROP);
            case 6 -> DOUBLE_CLICK;
            default -> UNKNOWN;
        };
    }
}
