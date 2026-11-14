package org.bukkit.event.inventory;

public enum InventoryType {
    CHEST(27, "Chest"), DISPENSER(9, "Dispenser"), DROPPER(9, "Dropper"), FURNACE(3, "Furnace"), WORKBENCH(10, "Crafting"),
    CRAFTING(5, "Crafting"), ENCHANTING(2, "Enchanting"), PLAYER(41, "Player"), CREATIVE(9, "Creative"), ANVIL(3, "Repairing"),
    ENDER_CHEST(27, "Ender Chest"), HOPPER(5, "Item Hopper"), SHULKER_BOX(27, "Shulker Box"), BARREL(27, "Barrel"),
    BLAST_FURNACE(3, "Blast Furnace"), SMOKER(3, "Smoker"), MERCHANT(3, "Villager"), BREWING(5, "Brewing");

    private final int size;
    private final String title;

    InventoryType(int size, String title) {
        this.size = size;
        this.title = title;
    }

    public int getDefaultSize() { return size; }

    public String getDefaultTitle() { return title; }

    public boolean isCreatable() { return this == CHEST || this == DISPENSER || this == DROPPER || this == HOPPER; }

    /** CreeMC: Fenster-Typ aus dem Server ("chest", "hopper" ...). */
    public static InventoryType fromCree(String t) {
        return switch (t == null ? "" : t) {
            case "hopper" -> HOPPER;
            case "dispenser" -> DISPENSER;
            case "furnace" -> FURNACE;
            case "anvil" -> ANVIL;
            case "enchanting" -> ENCHANTING;
            case "shulker_box" -> SHULKER_BOX;
            case "ender_chest" -> ENDER_CHEST;
            case "player" -> CRAFTING;
            case "workbench" -> WORKBENCH;
            default -> CHEST;
        };
    }

    public enum SlotType { RESULT, CRAFTING, ARMOR, CONTAINER, QUICKBAR, OUTSIDE, FUEL }
}
