# Unterstützte Bukkit-/Paper-API

Automatisch aus `creemc-api.jar` erzeugt: genau diese Klassen und Methoden gibt es in CreeMC.
Alles andere aus Bukkit/Paper fehlt (noch). Die eigene CreeMC-API steht in [PLUGIN-API.md](PLUGIN-API.md).

## `io.papermc.paper.command.brigadier`

### BasicCommand (interface)

```java
void execute(CommandSourceStack, String[])
Collection<String> suggest(CommandSourceStack, String[])
boolean canUse(CommandSender)
String permission()
```

### CommandSourceStack (interface)

```java
Location getLocation()
CommandSender getSender()
Entity getExecutor()
```

## `io.papermc.paper.event.player`

### AsyncChatEvent (class)

```java
AsyncChatEvent(boolean, Player, Set<Audience>, Component)
Component message()
void message(Component)
Component originalMessage()
Set<Audience> viewers()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

## `net.kyori.adventure.audience`

### Audience (interface)

```java
void sendLegacy(String)
void sendMessage(Component)
void sendMessage(ComponentLike)
void sendActionBar(Component)
void sendActionBar(ComponentLike)
```

## `net.kyori.adventure.bossbar`

### BossBar (class)

```java
float MIN_PROGRESS
float MAX_PROGRESS
BossBar bossBar(Component, float, BossBar$Color, BossBar$Overlay)
BossBar bossBar(Component, float, BossBar$Color, BossBar$Overlay, Set<BossBar$Flag>)
void creeSender(function.Consumer<Object[]>)
UUID creeId()
Set<UUID> creeViewers()
int creeFlags()
Component name()
BossBar name(Component)
float progress()
BossBar progress(float)
BossBar$Color color()
BossBar color(BossBar$Color)
BossBar$Overlay overlay()
BossBar overlay(BossBar$Overlay)
Set<BossBar$Flag> flags()
BossBar addFlag(BossBar$Flag)
BossBar removeFlag(BossBar$Flag)
boolean hasFlag(BossBar$Flag)
```

## `net.kyori.adventure.text`

### Component (interface)

```java
TextComponent empty()
TextComponent newline()
TextComponent space()
TextComponent text(String)
TextComponent text(Object)
TextComponent text(int)
TextComponent text(long)
TextComponent text(double)
TextComponent text(boolean)
TextComponent text(String, TextColor)
TextComponent text(String, TextColor, ..)
TextComponent$Builder text()
TextComponent translatable(String)
TextComponent join(Component, Iterable<? extends ComponentLike>)
Component append(Component)
Component append(ComponentLike)
Component color(TextColor)
TextColor color()
Component decorate(TextDecoration)
Component decoration(TextDecoration, boolean)
boolean hasDecoration(TextDecoration)
List<Component> children()
Component asComponent()
String toLegacy()
String toPlain()
```

### ComponentLike (interface)

```java
Component asComponent()
```

### TextComponent (interface)

```java
String content()
TextComponent content(String)
TextComponent append(Component)
TextComponent append(ComponentLike)
TextComponent color(TextColor)
TextComponent decorate(TextDecoration)
TextComponent decoration(TextDecoration, boolean)
Component decoration(TextDecoration, boolean)
Component decorate(TextDecoration)
Component color(TextColor)
Component append(ComponentLike)
Component append(Component)
```

### TextComponentImpl (class)

```java
String content()
TextComponent content(String)
TextComponent append(Component)
TextComponent append(ComponentLike)
TextComponent color(TextColor)
TextColor color()
TextComponent decorate(TextDecoration)
TextComponent decoration(TextDecoration, boolean)
boolean hasDecoration(TextDecoration)
List<Component> children()
String toLegacy()
String toPlain()
String toString()
boolean equals(Object)
int hashCode()
Component decoration(TextDecoration, boolean)
Component decorate(TextDecoration)
Component color(TextColor)
Component append(ComponentLike)
Component append(Component)
```

## `net.kyori.adventure.text.format`

### NamedTextColor (class)

```java
NamedTextColor BLACK
NamedTextColor DARK_BLUE
NamedTextColor DARK_GREEN
NamedTextColor DARK_AQUA
NamedTextColor DARK_RED
NamedTextColor DARK_PURPLE
NamedTextColor GOLD
NamedTextColor GRAY
NamedTextColor DARK_GRAY
NamedTextColor BLUE
NamedTextColor GREEN
NamedTextColor AQUA
NamedTextColor RED
NamedTextColor LIGHT_PURPLE
NamedTextColor YELLOW
NamedTextColor WHITE
int value()
NamedTextColor namedColor(int)
NamedTextColor nearestTo(TextColor)
NamedTextColor byName(String)
char legacyChar()
String toString()
```

### TextColor (interface)

```java
int value()
int red()
int green()
int blue()
String asHexString()
TextColor color(int)
TextColor color(int, int, int)
TextColor fromHexString(String)
TextColor fromCSSHexString(String)
char legacyCode(TextColor)
```

### TextDecoration (class)

```java
TextDecoration OBFUSCATED
TextDecoration BOLD
TextDecoration STRIKETHROUGH
TextDecoration UNDERLINED
TextDecoration ITALIC
char code
TextDecoration[] values()
TextDecoration valueOf(String)
```

## `net.kyori.adventure.text.logger.slf4j`

### ComponentLogger (interface)

```java
void info(Component)
void warn(Component)
void error(Component)
```

## `net.kyori.adventure.text.minimessage`

### MiniMessage (interface)

```java
MiniMessage miniMessage()
Component deserialize(String)
String serialize(Component)
String stripTags(String)
String escapeTags(String)
Object serialize(Component)
Component deserialize(Object)
```

## `net.kyori.adventure.text.serializer`

### ComponentSerializer (interface)

```java
O deserialize(R)
R serialize(I)
```

## `net.kyori.adventure.text.serializer.legacy`

### LegacyComponentSerializer (interface)

```java
char SECTION_CHAR
char AMPERSAND_CHAR
LegacyComponentSerializer legacySection()
LegacyComponentSerializer legacyAmpersand()
LegacyComponentSerializer legacy(char)
TextComponent deserialize(String)
String serialize(Component)
Object serialize(Component)
Component deserialize(Object)
```

## `net.kyori.adventure.text.serializer.plain`

### PlainTextComponentSerializer (interface)

```java
PlainTextComponentSerializer plainText()
TextComponent deserialize(String)
String serialize(Component)
Object serialize(Component)
Component deserialize(Object)
```

## `org.bukkit`

### Bukkit (class)

```java
Server getServer()
void setServer(Server)
String getName()
String getVersion()
String getBukkitVersion()
String getMinecraftVersion()
Collection<? extends Player> getOnlinePlayers()
int getMaxPlayers()
int getPort()
String getIp()
String getMotd()
boolean getOnlineMode()
int getViewDistance()
GameMode getDefaultGameMode()
Player getPlayer(String)
Player getPlayerExact(String)
Player getPlayer(UUID)
List<Player> matchPlayer(String)
OfflinePlayer getOfflinePlayer(String)
OfflinePlayer getOfflinePlayer(UUID)
PluginManager getPluginManager()
BukkitScheduler getScheduler()
World getWorld(String)
World getWorld(UUID)
List<World> getWorlds()
int broadcastMessage(String)
int broadcast(String, String)
int broadcast(Component)
ConsoleCommandSender getConsoleSender()
boolean dispatchCommand(CommandSender, String)
PluginCommand getPluginCommand(String)
logging.Logger getLogger()
boolean isPrimaryThread()
void shutdown()
double[] getTPS()
void reload()
void savePlayers()
Inventory createInventory(InventoryHolder, int)
Inventory createInventory(InventoryHolder, int, String)
Inventory createInventory(InventoryHolder, int, Component)
Inventory createInventory(InventoryHolder, InventoryType)
Inventory createInventory(InventoryHolder, InventoryType, String)
Inventory createInventory(InventoryHolder, InventoryType, Component)
BossBar createBossBar(String, BarColor, BarStyle, ..)
ScoreboardManager getScoreboardManager()
ItemFactory getItemFactory()
```

### ChatColor (class)

```java
ChatColor BLACK
ChatColor DARK_BLUE
ChatColor DARK_GREEN
ChatColor DARK_AQUA
ChatColor DARK_RED
ChatColor DARK_PURPLE
ChatColor GOLD
ChatColor GRAY
ChatColor DARK_GRAY
ChatColor BLUE
ChatColor GREEN
ChatColor AQUA
ChatColor RED
ChatColor LIGHT_PURPLE
ChatColor YELLOW
ChatColor WHITE
ChatColor MAGIC
ChatColor BOLD
ChatColor STRIKETHROUGH
ChatColor UNDERLINE
ChatColor ITALIC
ChatColor RESET
char COLOR_CHAR
ChatColor[] values()
ChatColor valueOf(String)
char getChar()
boolean isFormat()
boolean isColor()
String toString()
ChatColor getByChar(char)
ChatColor getByChar(String)
String stripColor(String)
String translateAlternateColorCodes(char, String)
String getLastColors(String)
```

### Color (class)

```java
Color WHITE
Color SILVER
Color GRAY
Color BLACK
Color RED
Color MAROON
Color YELLOW
Color OLIVE
Color LIME
Color GREEN
Color AQUA
Color TEAL
Color BLUE
Color NAVY
Color FUCHSIA
Color PURPLE
Color ORANGE
Color fromRGB(int)
Color fromRGB(int, int, int)
int asRGB()
int getRed()
int getGreen()
int getBlue()
boolean equals(Object)
int hashCode()
```

### GameMode (class)

```java
GameMode SURVIVAL
GameMode CREATIVE
GameMode ADVENTURE
GameMode SPECTATOR
GameMode[] values()
GameMode valueOf(String)
int getValue()
GameMode getByValue(int)
```

### GameRule (class)

```java
GameRule<Boolean> ANNOUNCE_ADVANCEMENTS
GameRule<Boolean> BLOCK_EXPLOSION_DROP_DECAY
GameRule<Boolean> COMMAND_BLOCK_OUTPUT
GameRule<Boolean> DISABLE_ELYTRA_MOVEMENT_CHECK
GameRule<Boolean> DISABLE_RAIDS
GameRule<Boolean> DO_DAYLIGHT_CYCLE
GameRule<Boolean> DO_ENTITY_DROPS
GameRule<Boolean> DO_FIRE_TICK
GameRule<Boolean> DO_IMMEDIATE_RESPAWN
GameRule<Boolean> DO_INSOMNIA
GameRule<Boolean> DO_LIMITED_CRAFTING
GameRule<Boolean> DO_MOB_LOOT
GameRule<Boolean> DO_MOB_SPAWNING
GameRule<Boolean> DO_PATROL_SPAWNING
GameRule<Boolean> DO_TILE_DROPS
GameRule<Boolean> DO_TRADER_SPAWNING
GameRule<Boolean> DO_VINES_SPREAD
GameRule<Boolean> DO_WARDEN_SPAWNING
GameRule<Boolean> DO_WEATHER_CYCLE
GameRule<Boolean> DROWNING_DAMAGE
GameRule<Boolean> ENDER_PEARLS_VANISH_ON_DEATH
GameRule<Boolean> FALL_DAMAGE
GameRule<Boolean> FIRE_DAMAGE
GameRule<Boolean> FORGIVE_DEAD_PLAYERS
GameRule<Boolean> FREEZE_DAMAGE
GameRule<Boolean> GLOBAL_SOUND_EVENTS
GameRule<Boolean> KEEP_INVENTORY
GameRule<Boolean> LAVA_SOURCE_CONVERSION
GameRule<Boolean> LOG_ADMIN_COMMANDS
GameRule<Boolean> MOB_EXPLOSION_DROP_DECAY
GameRule<Boolean> MOB_GRIEFING
GameRule<Boolean> NATURAL_REGENERATION
GameRule<Boolean> PROJECTILES_CAN_BREAK_BLOCKS
GameRule<Boolean> PVP
GameRule<Boolean> REDUCED_DEBUG_INFO
GameRule<Boolean> SEND_COMMAND_FEEDBACK
GameRule<Boolean> SHOW_DEATH_MESSAGES
GameRule<Boolean> SPAWN_MONSTERS
GameRule<Boolean> SPECTATORS_GENERATE_CHUNKS
GameRule<Boolean> TNT_EXPLODES
GameRule<Boolean> TNT_EXPLOSION_DROP_DECAY
GameRule<Boolean> UNIVERSAL_ANGER
GameRule<Boolean> WATER_SOURCE_CONVERSION
GameRule<Integer> MAX_COMMAND_CHAIN_LENGTH
GameRule<Integer> MAX_ENTITY_CRAMMING
GameRule<Integer> PLAYERS_SLEEPING_PERCENTAGE
GameRule<Integer> RANDOM_TICK_SPEED
GameRule<Integer> SNOW_ACCUMULATION_HEIGHT
GameRule<Integer> SPAWN_CHUNK_RADIUS
GameRule<Integer> SPAWN_RADIUS
String getName()
Class<T> getType()
NamespacedKey getKey()
GameRule<?> getByName(String)
GameRule<?>[] values()
String toString()
```

### Location (class)

```java
Location(World, double, double, double)
Location(World, double, double, double, float, float)
World getWorld()
void setWorld(World)
boolean isWorldLoaded()
Block getBlock()
double getX()
double getY()
double getZ()
void setX(double)
void setY(double)
void setZ(double)
float getYaw()
float getPitch()
void setYaw(float)
void setPitch(float)
int getBlockX()
int getBlockY()
int getBlockZ()
Location add(double, double, double)
Location add(Location)
Location add(Vector)
Location subtract(double, double, double)
Location subtract(Location)
Location subtract(Vector)
Location multiply(double)
Location zero()
double length()
double distanceSquared(Location)
double distance(Location)
Vector toVector()
Vector getDirection()
Location setDirection(Vector)
Location clone()
boolean equals(Object)
int hashCode()
String toString()
Object clone()
```

### Material (class)

```java
Material ABANDONED_CAMP_MAP
Material ACACIA_BOAT
Material ACACIA_BUTTON
Material ACACIA_CHEST_BOAT
Material ACACIA_DOOR
Material ACACIA_FENCE
Material ACACIA_FENCE_GATE
Material ACACIA_HANGING_SIGN
Material ACACIA_LEAVES
Material ACACIA_LOG
Material ACACIA_PLANKS
Material ACACIA_PRESSURE_PLATE
Material ACACIA_SAPLING
Material ACACIA_SHELF
Material ACACIA_SIGN
Material ACACIA_SLAB
Material ACACIA_STAIRS
Material ACACIA_TRAPDOOR
Material ACACIA_WALL_HANGING_SIGN
Material ACACIA_WALL_SIGN
Material ACACIA_WOOD
Material ACTIVATOR_RAIL
Material AIR
Material ALLAY_SPAWN_EGG
Material ALLIUM
Material AMETHYST_BLOCK
Material AMETHYST_CLUSTER
Material AMETHYST_SHARD
Material ANCIENT_DEBRIS
Material ANDESITE
Material ANDESITE_SLAB
Material ANDESITE_STAIRS
Material ANDESITE_WALL
Material ANGLER_POTTERY_SHERD
Material ANVIL
Material APPLE
Material ARCHER_POTTERY_SHERD
Material ARMADILLO_SCUTE
Material ARMADILLO_SPAWN_EGG
Material ARMOR_STAND
Material ARMS_UP_POTTERY_SHERD
Material ARROW
Material ATTACHED_MELON_STEM
Material ATTACHED_PUMPKIN_STEM
Material AXOLOTL_BUCKET
Material AXOLOTL_SPAWN_EGG
Material AZALEA
Material AZALEA_LEAVES
Material AZURE_BLUET
Material BAKED_POTATO
Material BAMBOO
Material BAMBOO_BLOCK
Material BAMBOO_BUTTON
Material BAMBOO_CHEST_RAFT
Material BAMBOO_DOOR
Material BAMBOO_FENCE
Material BAMBOO_FENCE_GATE
Material BAMBOO_HANGING_SIGN
Material BAMBOO_MOSAIC
Material BAMBOO_MOSAIC_SLAB
Material BAMBOO_MOSAIC_STAIRS
Material BAMBOO_PLANKS
Material BAMBOO_PRESSURE_PLATE
Material BAMBOO_RAFT
Material BAMBOO_SAPLING
Material BAMBOO_SHELF
Material BAMBOO_SIGN
Material BAMBOO_SLAB
Material BAMBOO_STAIRS
Material BAMBOO_TRAPDOOR
Material BAMBOO_WALL_HANGING_SIGN
Material BAMBOO_WALL_SIGN
Material BARREL
Material BARRIER
Material BASALT
Material BAT_SPAWN_EGG
Material BEACON
Material BEDROCK
Material BEE_NEST
Material BEE_SPAWN_EGG
Material BEEF
Material BEEHIVE
Material BEETROOT
Material BEETROOT_SEEDS
Material BEETROOT_SOUP
Material BEETROOTS
Material BELL
Material BIG_DRIPLEAF
Material BIG_DRIPLEAF_STEM
Material BIRCH_BOAT
Material BIRCH_BUTTON
Material BIRCH_CHEST_BOAT
Material BIRCH_DOOR
Material BIRCH_FENCE
Material BIRCH_FENCE_GATE
Material BIRCH_HANGING_SIGN
Material BIRCH_LEAVES
Material BIRCH_LOG
Material BIRCH_PLANKS
Material BIRCH_PRESSURE_PLATE
Material BIRCH_SAPLING
Material BIRCH_SHELF
Material BIRCH_SIGN
Material BIRCH_SLAB
Material BIRCH_STAIRS
Material BIRCH_TRAPDOOR
Material BIRCH_WALL_HANGING_SIGN
Material BIRCH_WALL_SIGN
Material BIRCH_WOOD
Material BLACK_BANNER
Material BLACK_BED
Material BLACK_BUNDLE
Material BLACK_CANDLE
Material BLACK_CANDLE_CAKE
Material BLACK_CARPET
Material BLACK_CONCRETE
Material BLACK_CONCRETE_POWDER
Material BLACK_CONCRETE_SLAB
Material BLACK_CONCRETE_STAIRS
Material BLACK_CUSHION
Material BLACK_DYE
Material BLACK_GLAZED_TERRACOTTA
Material BLACK_HARNESS
Material BLACK_SHULKER_BOX
Material BLACK_STAINED_GLASS
Material BLACK_STAINED_GLASS_PANE
Material BLACK_TERRACOTTA
Material BLACK_WALL_BANNER
Material BLACK_WOOL
Material BLACK_WOOL_SLAB
Material BLACK_WOOL_STAIRS
Material BLACKSTONE
Material BLACKSTONE_SLAB
Material BLACKSTONE_STAIRS
Material BLACKSTONE_WALL
Material BLADE_POTTERY_SHERD
Material BLAST_FURNACE
Material BLAZE_POWDER
Material BLAZE_ROD
Material BLAZE_SPAWN_EGG
Material BLUE_BANNER
Material BLUE_BED
Material BLUE_BUNDLE
Material BLUE_CANDLE
Material BLUE_CANDLE_CAKE
Material BLUE_CARPET
Material BLUE_CONCRETE
Material BLUE_CONCRETE_POWDER
Material BLUE_CONCRETE_SLAB
Material BLUE_CONCRETE_STAIRS
Material BLUE_CUSHION
Material BLUE_DYE
Material BLUE_EGG
Material BLUE_GLAZED_TERRACOTTA
Material BLUE_HARNESS
Material BLUE_ICE
Material BLUE_ORCHID
Material BLUE_SHULKER_BOX
Material BLUE_STAINED_GLASS
Material BLUE_STAINED_GLASS_PANE
Material BLUE_TERRACOTTA
Material BLUE_WALL_BANNER
Material BLUE_WOOL
Material BLUE_WOOL_SLAB
Material BLUE_WOOL_STAIRS
Material BOGGED_SPAWN_EGG
Material BOLT_ARMOR_TRIM_SMITHING_TEMPLATE
Material BONE
Material BONE_BLOCK
Material BONE_MEAL
Material BOOK
Material BOOKSHELF
Material BORDURE_INDENTED_BANNER_PATTERN
Material BOW
Material BOWL
Material BRAIN_CORAL
Material BRAIN_CORAL_BLOCK
Material BRAIN_CORAL_FAN
Material BRAIN_CORAL_WALL_FAN
Material BREAD
Material BREEZE_ROD
Material BREEZE_SPAWN_EGG
Material BREWER_POTTERY_SHERD
Material BREWING_STAND
Material BRICK
Material BRICK_SLAB
Material BRICK_STAIRS
Material BRICK_WALL
Material BRICKS
Material BROWN_BANNER
Material BROWN_BED
Material BROWN_BUNDLE
Material BROWN_CANDLE
Material BROWN_CANDLE_CAKE
Material BROWN_CARPET
Material BROWN_CONCRETE
Material BROWN_CONCRETE_POWDER
Material BROWN_CONCRETE_SLAB
Material BROWN_CONCRETE_STAIRS
Material BROWN_CUSHION
Material BROWN_DYE
Material BROWN_EGG
Material BROWN_GLAZED_TERRACOTTA
Material BROWN_HARNESS
Material BROWN_MUSHROOM
Material BROWN_MUSHROOM_BLOCK
Material BROWN_SHULKER_BOX
Material BROWN_STAINED_GLASS
Material BROWN_STAINED_GLASS_PANE
Material BROWN_TERRACOTTA
Material BROWN_WALL_BANNER
Material BROWN_WOOL
Material BROWN_WOOL_SLAB
Material BROWN_WOOL_STAIRS
Material BRUSH
Material BUBBLE_COLUMN
Material BUBBLE_CORAL
Material BUBBLE_CORAL_BLOCK
Material BUBBLE_CORAL_FAN
Material BUBBLE_CORAL_WALL_FAN
Material BUCKET
Material BUDDING_AMETHYST
Material BUNDLE
Material BURIED_ANCIENT_CITY_MAP
Material BURIED_MINESHAFT_MAP
Material BURIED_TREASURE_MAP
Material BURIED_TRIAL_CHAMBERS_MAP
Material BURN_POTTERY_SHERD
Material BUSH
Material CACTUS
Material CACTUS_FLOWER
Material CAKE
Material CALCITE
Material CALIBRATED_SCULK_SENSOR
Material CAMEL_HUSK_SPAWN_EGG
Material CAMEL_SPAWN_EGG
Material CAMPFIRE
Material CANDLE
Material CANDLE_CAKE
Material CARROT
Material CARROT_ON_A_STICK
Material CARROTS
Material CARTOGRAPHY_TABLE
Material CARVED_PUMPKIN
Material CAT_SPAWN_EGG
Material CAULDRON
Material CAVE_AIR
Material CAVE_SPIDER_SPAWN_EGG
Material CAVE_VINES
Material CAVE_VINES_PLANT
Material CHAIN_COMMAND_BLOCK
Material CHAINMAIL_BOOTS
Material CHAINMAIL_CHESTPLATE
Material CHAINMAIL_HELMET
Material CHAINMAIL_LEGGINGS
Material CHARCOAL
Material CHERRY_BOAT
Material CHERRY_BUTTON
Material CHERRY_CHEST_BOAT
Material CHERRY_DOOR
Material CHERRY_FENCE
Material CHERRY_FENCE_GATE
Material CHERRY_HANGING_SIGN
Material CHERRY_LEAVES
Material CHERRY_LOG
Material CHERRY_PLANKS
Material CHERRY_PRESSURE_PLATE
Material CHERRY_SAPLING
Material CHERRY_SHELF
Material CHERRY_SIGN
Material CHERRY_SLAB
Material CHERRY_STAIRS
Material CHERRY_TRAPDOOR
Material CHERRY_WALL_HANGING_SIGN
Material CHERRY_WALL_SIGN
Material CHERRY_WOOD
Material CHEST
Material CHEST_MINECART
Material CHICKEN
Material CHICKEN_SPAWN_EGG
Material CHIPPED_ANVIL
Material CHISELED_BOOKSHELF
Material CHISELED_CINNABAR
Material CHISELED_COPPER
Material CHISELED_DEEPSLATE
Material CHISELED_NETHER_BRICKS
Material CHISELED_POLISHED_BLACKSTONE
Material CHISELED_QUARTZ_BLOCK
Material CHISELED_RED_SANDSTONE
Material CHISELED_RESIN_BRICKS
Material CHISELED_SANDSTONE
Material CHISELED_STONE_BRICKS
Material CHISELED_SULFUR
Material CHISELED_TUFF
Material CHISELED_TUFF_BRICKS
Material CHORUS_FLOWER
Material CHORUS_FRUIT
Material CHORUS_PLANT
Material CINNABAR
Material CINNABAR_BRICK_SLAB
Material CINNABAR_BRICK_STAIRS
Material CINNABAR_BRICK_WALL
Material CINNABAR_BRICKS
Material CINNABAR_SLAB
Material CINNABAR_STAIRS
Material CINNABAR_WALL
Material CLAY
Material CLAY_BALL
Material CLOCK
Material CLOSED_EYEBLOSSOM
Material COAL
Material COAL_BLOCK
Material COAL_ORE
Material COARSE_DIRT
Material COAST_ARMOR_TRIM_SMITHING_TEMPLATE
Material COBBLED_DEEPSLATE
Material COBBLED_DEEPSLATE_SLAB
Material COBBLED_DEEPSLATE_STAIRS
Material COBBLED_DEEPSLATE_WALL
Material COBBLESTONE
Material COBBLESTONE_SLAB
Material COBBLESTONE_STAIRS
Material COBBLESTONE_WALL
Material COBWEB
Material COCOA
Material COCOA_BEANS
Material COD
Material COD_BUCKET
Material COD_SPAWN_EGG
Material COMMAND_BLOCK
Material COMMAND_BLOCK_MINECART
Material COMPARATOR
Material COMPASS
Material COMPOSTER
Material CONDUIT
Material COOKED_BEEF
Material COOKED_CHICKEN
Material COOKED_COD
Material COOKED_MUTTON
Material COOKED_PORKCHOP
Material COOKED_RABBIT
Material COOKED_SALMON
Material COOKIE
Material COPPER_AXE
Material COPPER_BARS
Material COPPER_BLOCK
Material COPPER_BOOTS
Material COPPER_BULB
Material COPPER_CHAIN
Material COPPER_CHEST
Material COPPER_CHESTPLATE
Material COPPER_DOOR
Material COPPER_GOLEM_SPAWN_EGG
Material COPPER_GOLEM_STATUE
Material COPPER_GRATE
Material COPPER_HELMET
Material COPPER_HOE
Material COPPER_HORSE_ARMOR
Material COPPER_INGOT
Material COPPER_LANTERN
Material COPPER_LEGGINGS
Material COPPER_NAUTILUS_ARMOR
Material COPPER_NUGGET
Material COPPER_ORE
Material COPPER_PICKAXE
Material COPPER_SHOVEL
Material COPPER_SPEAR
Material COPPER_SWORD
Material COPPER_TORCH
Material COPPER_TRAPDOOR
Material COPPER_WALL_TORCH
Material CORNFLOWER
Material COW_SPAWN_EGG
Material CRACKED_DEEPSLATE_BRICKS
Material CRACKED_DEEPSLATE_TILES
Material CRACKED_NETHER_BRICKS
Material CRACKED_POLISHED_BLACKSTONE_BRICKS
Material CRACKED_STONE_BRICKS
Material CRAFTER
Material CRAFTING_TABLE
Material CREAKING_HEART
Material CREAKING_SPAWN_EGG
Material CREEPER_BANNER_PATTERN
Material CREEPER_HEAD
Material CREEPER_SPAWN_EGG
Material CREEPER_WALL_HEAD
Material CRIMSON_BUTTON
Material CRIMSON_DOOR
Material CRIMSON_FENCE
Material CRIMSON_FENCE_GATE
Material CRIMSON_FUNGUS
Material CRIMSON_HANGING_SIGN
Material CRIMSON_HYPHAE
Material CRIMSON_NYLIUM
Material CRIMSON_PLANKS
Material CRIMSON_PRESSURE_PLATE
Material CRIMSON_ROOTS
Material CRIMSON_SHELF
Material CRIMSON_SIGN
Material CRIMSON_SLAB
Material CRIMSON_STAIRS
Material CRIMSON_STEM
Material CRIMSON_TRAPDOOR
Material CRIMSON_WALL_HANGING_SIGN
Material CRIMSON_WALL_SIGN
Material CROSSBOW
Material CRYING_OBSIDIAN
Material CUT_COPPER
Material CUT_COPPER_SLAB
Material CUT_COPPER_STAIRS
Material CUT_RED_SANDSTONE
Material CUT_RED_SANDSTONE_SLAB
Material CUT_SANDSTONE
Material CUT_SANDSTONE_SLAB
Material CYAN_BANNER
Material CYAN_BED
Material CYAN_BUNDLE
Material CYAN_CANDLE
Material CYAN_CANDLE_CAKE
Material CYAN_CARPET
Material CYAN_CONCRETE
Material CYAN_CONCRETE_POWDER
Material CYAN_CONCRETE_SLAB
Material CYAN_CONCRETE_STAIRS
Material CYAN_CUSHION
Material CYAN_DYE
Material CYAN_GLAZED_TERRACOTTA
Material CYAN_HARNESS
Material CYAN_SHULKER_BOX
Material CYAN_STAINED_GLASS
Material CYAN_STAINED_GLASS_PANE
Material CYAN_TERRACOTTA
Material CYAN_WALL_BANNER
Material CYAN_WOOL
Material CYAN_WOOL_SLAB
Material CYAN_WOOL_STAIRS
Material DAMAGED_ANVIL
Material DANDELION
Material DANGER_POTTERY_SHERD
Material DARK_OAK_BOAT
Material DARK_OAK_BUTTON
Material DARK_OAK_CHEST_BOAT
Material DARK_OAK_DOOR
Material DARK_OAK_FENCE
Material DARK_OAK_FENCE_GATE
Material DARK_OAK_HANGING_SIGN
Material DARK_OAK_LEAVES
Material DARK_OAK_LOG
Material DARK_OAK_PLANKS
Material DARK_OAK_PRESSURE_PLATE
Material DARK_OAK_SAPLING
Material DARK_OAK_SHELF
Material DARK_OAK_SIGN
Material DARK_OAK_SLAB
Material DARK_OAK_STAIRS
Material DARK_OAK_TRAPDOOR
Material DARK_OAK_WALL_HANGING_SIGN
Material DARK_OAK_WALL_SIGN
Material DARK_OAK_WOOD
Material DARK_PRISMARINE
Material DARK_PRISMARINE_SLAB
Material DARK_PRISMARINE_STAIRS
Material DAYLIGHT_DETECTOR
Material DEAD_BRAIN_CORAL
Material DEAD_BRAIN_CORAL_BLOCK
Material DEAD_BRAIN_CORAL_FAN
Material DEAD_BRAIN_CORAL_WALL_FAN
Material DEAD_BUBBLE_CORAL
Material DEAD_BUBBLE_CORAL_BLOCK
Material DEAD_BUBBLE_CORAL_FAN
Material DEAD_BUBBLE_CORAL_WALL_FAN
Material DEAD_BUSH
Material DEAD_FIRE_CORAL
Material DEAD_FIRE_CORAL_BLOCK
Material DEAD_FIRE_CORAL_FAN
Material DEAD_FIRE_CORAL_WALL_FAN
Material DEAD_HORN_CORAL
Material DEAD_HORN_CORAL_BLOCK
Material DEAD_HORN_CORAL_FAN
Material DEAD_HORN_CORAL_WALL_FAN
Material DEAD_TUBE_CORAL
Material DEAD_TUBE_CORAL_BLOCK
Material DEAD_TUBE_CORAL_FAN
Material DEAD_TUBE_CORAL_WALL_FAN
Material DEBUG_STICK
Material DECORATED_POT
Material DEEPSLATE
Material DEEPSLATE_BRICK_SLAB
Material DEEPSLATE_BRICK_STAIRS
Material DEEPSLATE_BRICK_WALL
Material DEEPSLATE_BRICKS
Material DEEPSLATE_COAL_ORE
Material DEEPSLATE_COPPER_ORE
Material DEEPSLATE_DIAMOND_ORE
Material DEEPSLATE_EMERALD_ORE
Material DEEPSLATE_GOLD_ORE
Material DEEPSLATE_IRON_ORE
Material DEEPSLATE_LAPIS_ORE
Material DEEPSLATE_REDSTONE_ORE
Material DEEPSLATE_TILE_SLAB
Material DEEPSLATE_TILE_STAIRS
Material DEEPSLATE_TILE_WALL
Material DEEPSLATE_TILES
Material DESERT_PYRAMID_MAP
Material DESERT_VILLAGE_MAP
Material DETECTOR_RAIL
Material DIAMOND
Material DIAMOND_AXE
Material DIAMOND_BLOCK
Material DIAMOND_BOOTS
Material DIAMOND_CHESTPLATE
Material DIAMOND_HELMET
Material DIAMOND_HOE
Material DIAMOND_HORSE_ARMOR
Material DIAMOND_LEGGINGS
Material DIAMOND_NAUTILUS_ARMOR
Material DIAMOND_ORE
Material DIAMOND_PICKAXE
Material DIAMOND_SHOVEL
Material DIAMOND_SPEAR
Material DIAMOND_SWORD
Material DIORITE
Material DIORITE_SLAB
Material DIORITE_STAIRS
Material DIORITE_WALL
Material DIRT
Material DIRT_PATH
Material DISC_FRAGMENT_5
Material DISPENSER
Material DOLPHIN_SPAWN_EGG
Material DONKEY_SPAWN_EGG
Material DRAGON_BREATH
Material DRAGON_EGG
Material DRAGON_HEAD
Material DRAGON_WALL_HEAD
Material DRIED_GHAST
Material DRIED_KELP
Material DRIED_KELP_BLOCK
Material DRIPSTONE_BLOCK
Material DROPPER
Material DROWNED_SPAWN_EGG
Material DUNE_ARMOR_TRIM_SMITHING_TEMPLATE
Material ECHO_SHARD
Material EGG
Material ELDER_GUARDIAN_SPAWN_EGG
Material ELYTRA
Material EMERALD
Material EMERALD_BLOCK
Material EMERALD_ORE
Material ENCHANTED_BOOK
Material ENCHANTED_GOLDEN_APPLE
Material ENCHANTING_TABLE
Material END_CRYSTAL
Material END_GATEWAY
Material END_PORTAL
Material END_PORTAL_FRAME
Material END_ROD
Material END_STONE
Material END_STONE_BRICK_SLAB
Material END_STONE_BRICK_STAIRS
Material END_STONE_BRICK_WALL
Material END_STONE_BRICKS
Material ENDER_CHEST
Material ENDER_DRAGON_SPAWN_EGG
Material ENDER_EYE
Material ENDER_PEARL
Material ENDERMAN_SPAWN_EGG
Material ENDERMITE_SPAWN_EGG
Material EVOKER_SPAWN_EGG
Material EXPERIENCE_BOTTLE
Material EXPLORER_POTTERY_SHERD
Material EXPOSED_CHISELED_COPPER
Material EXPOSED_COPPER
Material EXPOSED_COPPER_BARS
Material EXPOSED_COPPER_BULB
Material EXPOSED_COPPER_CHAIN
Material EXPOSED_COPPER_CHEST
Material EXPOSED_COPPER_DOOR
Material EXPOSED_COPPER_GOLEM_STATUE
Material EXPOSED_COPPER_GRATE
Material EXPOSED_COPPER_LANTERN
Material EXPOSED_COPPER_TRAPDOOR
Material EXPOSED_CUT_COPPER
Material EXPOSED_CUT_COPPER_SLAB
Material EXPOSED_CUT_COPPER_STAIRS
Material EXPOSED_LIGHTNING_ROD
Material EYE_ARMOR_TRIM_SMITHING_TEMPLATE
Material FARMLAND
Material FEATHER
Material FERMENTED_SPIDER_EYE
Material FERN
Material FIELD_MASONED_BANNER_PATTERN
Material FILLED_MAP
Material FIRE
Material FIRE_CHARGE
Material FIRE_CORAL
Material FIRE_CORAL_BLOCK
Material FIRE_CORAL_FAN
Material FIRE_CORAL_WALL_FAN
Material FIREFLY_BUSH
Material FIREWORK_ROCKET
Material FIREWORK_STAR
Material FISHING_ROD
Material FLETCHING_TABLE
Material FLINT
Material FLINT_AND_STEEL
Material FLOW_ARMOR_TRIM_SMITHING_TEMPLATE
Material FLOW_BANNER_PATTERN
Material FLOW_POTTERY_SHERD
Material FLOWER_BANNER_PATTERN
Material FLOWER_POT
Material FLOWERING_AZALEA
Material FLOWERING_AZALEA_LEAVES
Material FOX_SPAWN_EGG
Material FRIEND_POTTERY_SHERD
Material FROG_SPAWN_EGG
Material FROGSPAWN
Material FROSTED_ICE
Material FURNACE
Material FURNACE_MINECART
Material GHAST_SPAWN_EGG
Material GHAST_TEAR
Material GILDED_BLACKSTONE
Material GLASS
Material GLASS_BOTTLE
Material GLASS_PANE
Material GLISTERING_MELON_SLICE
Material GLOBE_BANNER_PATTERN
Material GLOW_BERRIES
Material GLOW_INK_SAC
Material GLOW_ITEM_FRAME
Material GLOW_LICHEN
Material GLOW_SQUID_SPAWN_EGG
Material GLOWSTONE
Material GLOWSTONE_DUST
Material GOAT_HORN
Material GOAT_SPAWN_EGG
Material GOLD_BLOCK
Material GOLD_INGOT
Material GOLD_NUGGET
Material GOLD_ORE
Material GOLDEN_APPLE
Material GOLDEN_AXE
Material GOLDEN_BOOTS
Material GOLDEN_CARROT
Material GOLDEN_CHESTPLATE
Material GOLDEN_DANDELION
Material GOLDEN_HELMET
Material GOLDEN_HOE
Material GOLDEN_HORSE_ARMOR
Material GOLDEN_LEGGINGS
Material GOLDEN_NAUTILUS_ARMOR
Material GOLDEN_PICKAXE
Material GOLDEN_SHOVEL
Material GOLDEN_SPEAR
Material GOLDEN_SWORD
Material GRANITE
Material GRANITE_SLAB
Material GRANITE_STAIRS
Material GRANITE_WALL
Material GRASS_BLOCK
Material GRAVEL
Material GRAY_BANNER
Material GRAY_BED
Material GRAY_BUNDLE
Material GRAY_CANDLE
Material GRAY_CANDLE_CAKE
Material GRAY_CARPET
Material GRAY_CONCRETE
Material GRAY_CONCRETE_POWDER
Material GRAY_CONCRETE_SLAB
Material GRAY_CONCRETE_STAIRS
Material GRAY_CUSHION
Material GRAY_DYE
Material GRAY_GLAZED_TERRACOTTA
Material GRAY_HARNESS
Material GRAY_SHULKER_BOX
Material GRAY_STAINED_GLASS
Material GRAY_STAINED_GLASS_PANE
Material GRAY_TERRACOTTA
Material GRAY_WALL_BANNER
Material GRAY_WOOL
Material GRAY_WOOL_SLAB
Material GRAY_WOOL_STAIRS
Material GREEN_BANNER
Material GREEN_BED
Material GREEN_BUNDLE
Material GREEN_CANDLE
Material GREEN_CANDLE_CAKE
Material GREEN_CARPET
Material GREEN_CONCRETE
Material GREEN_CONCRETE_POWDER
Material GREEN_CONCRETE_SLAB
Material GREEN_CONCRETE_STAIRS
Material GREEN_CUSHION
Material GREEN_DYE
Material GREEN_GLAZED_TERRACOTTA
Material GREEN_HARNESS
Material GREEN_SHULKER_BOX
Material GREEN_STAINED_GLASS
Material GREEN_STAINED_GLASS_PANE
Material GREEN_TERRACOTTA
Material GREEN_WALL_BANNER
Material GREEN_WOOL
Material GREEN_WOOL_SLAB
Material GREEN_WOOL_STAIRS
Material GRINDSTONE
Material GUARDIAN_SPAWN_EGG
Material GUNPOWDER
Material GUSTER_BANNER_PATTERN
Material GUSTER_POTTERY_SHERD
Material HANGING_ROOTS
Material HAPPY_GHAST_SPAWN_EGG
Material HAY_BLOCK
Material HEART_OF_THE_SEA
Material HEART_POTTERY_SHERD
Material HEARTBREAK_POTTERY_SHERD
Material HEAVY_CORE
Material HEAVY_WEIGHTED_PRESSURE_PLATE
Material HOGLIN_SPAWN_EGG
Material HONEY_BLOCK
Material HONEY_BOTTLE
Material HONEYCOMB
Material HONEYCOMB_BLOCK
Material HOPPER
Material HOPPER_MINECART
Material HORN_CORAL
Material HORN_CORAL_BLOCK
Material HORN_CORAL_FAN
Material HORN_CORAL_WALL_FAN
Material HORSE_SPAWN_EGG
Material HOST_ARMOR_TRIM_SMITHING_TEMPLATE
Material HOWL_POTTERY_SHERD
Material HUSK_SPAWN_EGG
Material ICE
Material INFESTED_CHISELED_STONE_BRICKS
Material INFESTED_COBBLESTONE
Material INFESTED_CRACKED_STONE_BRICKS
Material INFESTED_DEEPSLATE
Material INFESTED_MOSSY_STONE_BRICKS
Material INFESTED_STONE
Material INFESTED_STONE_BRICKS
Material INK_SAC
Material IRON_AXE
Material IRON_BARS
Material IRON_BLOCK
Material IRON_BOOTS
Material IRON_CHAIN
Material IRON_CHESTPLATE
Material IRON_DOOR
Material IRON_GOLEM_SPAWN_EGG
Material IRON_HELMET
Material IRON_HOE
Material IRON_HORSE_ARMOR
Material IRON_INGOT
Material IRON_LEGGINGS
Material IRON_NAUTILUS_ARMOR
Material IRON_NUGGET
Material IRON_ORE
Material IRON_PICKAXE
Material IRON_SHOVEL
Material IRON_SPEAR
Material IRON_SWORD
Material IRON_TRAPDOOR
Material ITEM_FRAME
Material JACK_O_LANTERN
Material JIGSAW
Material JUKEBOX
Material JUNGLE_BOAT
Material JUNGLE_BUTTON
Material JUNGLE_CHEST_BOAT
Material JUNGLE_DOOR
Material JUNGLE_FENCE
Material JUNGLE_FENCE_GATE
Material JUNGLE_HANGING_SIGN
Material JUNGLE_LEAVES
Material JUNGLE_LOG
Material JUNGLE_PLANKS
Material JUNGLE_PRESSURE_PLATE
Material JUNGLE_PYRAMID_MAP
Material JUNGLE_SAPLING
Material JUNGLE_SHELF
Material JUNGLE_SIGN
Material JUNGLE_SLAB
Material JUNGLE_STAIRS
Material JUNGLE_TRAPDOOR
Material JUNGLE_WALL_HANGING_SIGN
Material JUNGLE_WALL_SIGN
Material JUNGLE_WOOD
Material KELP
Material KELP_PLANT
Material KNOWLEDGE_BOOK
Material LADDER
Material LANTERN
Material LAPIS_BLOCK
Material LAPIS_LAZULI
Material LAPIS_ORE
Material LARGE_AMETHYST_BUD
Material LARGE_FERN
Material LAVA
Material LAVA_BUCKET
Material LAVA_CAULDRON
Material LEAD
Material LEAF_LITTER
Material LEATHER
Material LEATHER_BOOTS
Material LEATHER_CHESTPLATE
Material LEATHER_HELMET
Material LEATHER_HORSE_ARMOR
Material LEATHER_LEGGINGS
Material LECTERN
Material LEVER
Material LIGHT
Material LIGHT_BLUE_BANNER
Material LIGHT_BLUE_BED
Material LIGHT_BLUE_BUNDLE
Material LIGHT_BLUE_CANDLE
Material LIGHT_BLUE_CANDLE_CAKE
Material LIGHT_BLUE_CARPET
Material LIGHT_BLUE_CONCRETE
Material LIGHT_BLUE_CONCRETE_POWDER
Material LIGHT_BLUE_CONCRETE_SLAB
Material LIGHT_BLUE_CONCRETE_STAIRS
Material LIGHT_BLUE_CUSHION
Material LIGHT_BLUE_DYE
Material LIGHT_BLUE_GLAZED_TERRACOTTA
Material LIGHT_BLUE_HARNESS
Material LIGHT_BLUE_SHULKER_BOX
Material LIGHT_BLUE_STAINED_GLASS
Material LIGHT_BLUE_STAINED_GLASS_PANE
Material LIGHT_BLUE_TERRACOTTA
Material LIGHT_BLUE_WALL_BANNER
Material LIGHT_BLUE_WOOL
Material LIGHT_BLUE_WOOL_SLAB
Material LIGHT_BLUE_WOOL_STAIRS
Material LIGHT_GRAY_BANNER
Material LIGHT_GRAY_BED
Material LIGHT_GRAY_BUNDLE
Material LIGHT_GRAY_CANDLE
Material LIGHT_GRAY_CANDLE_CAKE
Material LIGHT_GRAY_CARPET
Material LIGHT_GRAY_CONCRETE
Material LIGHT_GRAY_CONCRETE_POWDER
Material LIGHT_GRAY_CONCRETE_SLAB
Material LIGHT_GRAY_CONCRETE_STAIRS
Material LIGHT_GRAY_CUSHION
Material LIGHT_GRAY_DYE
Material LIGHT_GRAY_GLAZED_TERRACOTTA
Material LIGHT_GRAY_HARNESS
Material LIGHT_GRAY_SHULKER_BOX
Material LIGHT_GRAY_STAINED_GLASS
Material LIGHT_GRAY_STAINED_GLASS_PANE
Material LIGHT_GRAY_TERRACOTTA
Material LIGHT_GRAY_WALL_BANNER
Material LIGHT_GRAY_WOOL
Material LIGHT_GRAY_WOOL_SLAB
Material LIGHT_GRAY_WOOL_STAIRS
Material LIGHT_WEIGHTED_PRESSURE_PLATE
Material LIGHTNING_ROD
Material LILAC
Material LILY_OF_THE_VALLEY
Material LILY_PAD
Material LIME_BANNER
Material LIME_BED
Material LIME_BUNDLE
Material LIME_CANDLE
Material LIME_CANDLE_CAKE
Material LIME_CARPET
Material LIME_CONCRETE
Material LIME_CONCRETE_POWDER
Material LIME_CONCRETE_SLAB
Material LIME_CONCRETE_STAIRS
Material LIME_CUSHION
Material LIME_DYE
Material LIME_GLAZED_TERRACOTTA
Material LIME_HARNESS
Material LIME_SHULKER_BOX
Material LIME_STAINED_GLASS
Material LIME_STAINED_GLASS_PANE
Material LIME_TERRACOTTA
Material LIME_WALL_BANNER
Material LIME_WOOL
Material LIME_WOOL_SLAB
Material LIME_WOOL_STAIRS
Material LINGERING_POTION
Material LLAMA_SPAWN_EGG
Material LODESTONE
Material LOOM
Material MACE
Material MAGENTA_BANNER
Material MAGENTA_BED
Material MAGENTA_BUNDLE
Material MAGENTA_CANDLE
Material MAGENTA_CANDLE_CAKE
Material MAGENTA_CARPET
Material MAGENTA_CONCRETE
Material MAGENTA_CONCRETE_POWDER
Material MAGENTA_CONCRETE_SLAB
Material MAGENTA_CONCRETE_STAIRS
Material MAGENTA_CUSHION
Material MAGENTA_DYE
Material MAGENTA_GLAZED_TERRACOTTA
Material MAGENTA_HARNESS
Material MAGENTA_SHULKER_BOX
Material MAGENTA_STAINED_GLASS
Material MAGENTA_STAINED_GLASS_PANE
Material MAGENTA_TERRACOTTA
Material MAGENTA_WALL_BANNER
Material MAGENTA_WOOL
Material MAGENTA_WOOL_SLAB
Material MAGENTA_WOOL_STAIRS
Material MAGMA_BLOCK
Material MAGMA_CREAM
Material MAGMA_CUBE_SPAWN_EGG
Material MANGROVE_BOAT
Material MANGROVE_BUTTON
Material MANGROVE_CHEST_BOAT
Material MANGROVE_DOOR
Material MANGROVE_FENCE
Material MANGROVE_FENCE_GATE
Material MANGROVE_HANGING_SIGN
Material MANGROVE_LEAVES
Material MANGROVE_LOG
Material MANGROVE_PLANKS
Material MANGROVE_PRESSURE_PLATE
Material MANGROVE_PROPAGULE
Material MANGROVE_ROOTS
Material MANGROVE_SHELF
Material MANGROVE_SIGN
Material MANGROVE_SLAB
Material MANGROVE_STAIRS
Material MANGROVE_TRAPDOOR
Material MANGROVE_WALL_HANGING_SIGN
Material MANGROVE_WALL_SIGN
Material MANGROVE_WOOD
Material MAP
Material MEDIUM_AMETHYST_BUD
Material MELON
Material MELON_SEEDS
Material MELON_SLICE
Material MELON_STEM
Material MILK_BUCKET
Material MINECART
Material MINER_POTTERY_SHERD
Material MOJANG_BANNER_PATTERN
Material MOOSHROOM_SPAWN_EGG
Material MOSS_BLOCK
Material MOSS_CARPET
Material MOSSY_COBBLESTONE
Material MOSSY_COBBLESTONE_SLAB
Material MOSSY_COBBLESTONE_STAIRS
Material MOSSY_COBBLESTONE_WALL
Material MOSSY_STONE_BRICK_SLAB
Material MOSSY_STONE_BRICK_STAIRS
Material MOSSY_STONE_BRICK_WALL
Material MOSSY_STONE_BRICKS
Material MOURNER_POTTERY_SHERD
Material MOVING_PISTON
Material MUD
Material MUD_BRICK_SLAB
Material MUD_BRICK_STAIRS
Material MUD_BRICK_WALL
Material MUD_BRICKS
Material MUDDY_MANGROVE_ROOTS
Material MULE_SPAWN_EGG
Material MUSHROOM_STEM
Material MUSHROOM_STEW
Material MUSIC_DISC_11
Material MUSIC_DISC_13
Material MUSIC_DISC_5
Material MUSIC_DISC_BLOCKS
Material MUSIC_DISC_BOUNCE
Material MUSIC_DISC_CAT
Material MUSIC_DISC_CHIRP
Material MUSIC_DISC_CREATOR
Material MUSIC_DISC_CREATOR_MUSIC_BOX
Material MUSIC_DISC_FAR
Material MUSIC_DISC_LAVA_CHICKEN
Material MUSIC_DISC_MALL
Material MUSIC_DISC_MELLOHI
Material MUSIC_DISC_OTHERSIDE
Material MUSIC_DISC_PIGSTEP
Material MUSIC_DISC_PRECIPICE
Material MUSIC_DISC_RELIC
Material MUSIC_DISC_STAL
Material MUSIC_DISC_STRAD
Material MUSIC_DISC_TEARS
Material MUSIC_DISC_WAIT
Material MUSIC_DISC_WARD
Material MUTTON
Material MYCELIUM
Material NAME_TAG
Material NAUTILUS_SHELL
Material NAUTILUS_SPAWN_EGG
Material NETHER_BRICK
Material NETHER_BRICK_FENCE
Material NETHER_BRICK_SLAB
Material NETHER_BRICK_STAIRS
Material NETHER_BRICK_WALL
Material NETHER_BRICKS
Material NETHER_GOLD_ORE
Material NETHER_PORTAL
Material NETHER_QUARTZ_ORE
Material NETHER_SPROUTS
Material NETHER_STAR
Material NETHER_WART
Material NETHER_WART_BLOCK
Material NETHERITE_AXE
Material NETHERITE_BLOCK
Material NETHERITE_BOOTS
Material NETHERITE_CHESTPLATE
Material NETHERITE_HELMET
Material NETHERITE_HOE
Material NETHERITE_HORSE_ARMOR
Material NETHERITE_INGOT
Material NETHERITE_LEGGINGS
Material NETHERITE_NAUTILUS_ARMOR
Material NETHERITE_PICKAXE
Material NETHERITE_SCRAP
Material NETHERITE_SHOVEL
Material NETHERITE_SPEAR
Material NETHERITE_SWORD
Material NETHERITE_UPGRADE_SMITHING_TEMPLATE
Material NETHERRACK
Material NOTE_BLOCK
Material OAK_BOAT
Material OAK_BUTTON
Material OAK_CHEST_BOAT
Material OAK_DOOR
Material OAK_FENCE
Material OAK_FENCE_GATE
Material OAK_HANGING_SIGN
Material OAK_LEAVES
Material OAK_LOG
Material OAK_PLANKS
Material OAK_PRESSURE_PLATE
Material OAK_SAPLING
Material OAK_SHELF
Material OAK_SIGN
Material OAK_SLAB
Material OAK_STAIRS
Material OAK_TRAPDOOR
Material OAK_WALL_HANGING_SIGN
Material OAK_WALL_SIGN
Material OAK_WOOD
Material OBSERVER
Material OBSIDIAN
Material OCEAN_MONUMENT_MAP
Material OCELOT_SPAWN_EGG
Material OCHRE_FROGLIGHT
Material OMINOUS_BOTTLE
Material OMINOUS_TRIAL_KEY
Material OPEN_EYEBLOSSOM
Material ORANGE_BANNER
Material ORANGE_BED
Material ORANGE_BUNDLE
Material ORANGE_CANDLE
Material ORANGE_CANDLE_CAKE
Material ORANGE_CARPET
Material ORANGE_CONCRETE
Material ORANGE_CONCRETE_POWDER
Material ORANGE_CONCRETE_SLAB
Material ORANGE_CONCRETE_STAIRS
Material ORANGE_CUSHION
Material ORANGE_DYE
Material ORANGE_GLAZED_TERRACOTTA
Material ORANGE_HARNESS
Material ORANGE_POPLAR_LEAVES
Material ORANGE_SHULKER_BOX
Material ORANGE_STAINED_GLASS
Material ORANGE_STAINED_GLASS_PANE
Material ORANGE_TERRACOTTA
Material ORANGE_TULIP
Material ORANGE_WALL_BANNER
Material ORANGE_WOOL
Material ORANGE_WOOL_SLAB
Material ORANGE_WOOL_STAIRS
Material OXEYE_DAISY
Material OXIDIZED_CHISELED_COPPER
Material OXIDIZED_COPPER
Material OXIDIZED_COPPER_BARS
Material OXIDIZED_COPPER_BULB
Material OXIDIZED_COPPER_CHAIN
Material OXIDIZED_COPPER_CHEST
Material OXIDIZED_COPPER_DOOR
Material OXIDIZED_COPPER_GOLEM_STATUE
Material OXIDIZED_COPPER_GRATE
Material OXIDIZED_COPPER_LANTERN
Material OXIDIZED_COPPER_TRAPDOOR
Material OXIDIZED_CUT_COPPER
Material OXIDIZED_CUT_COPPER_SLAB
Material OXIDIZED_CUT_COPPER_STAIRS
Material OXIDIZED_LIGHTNING_ROD
Material PACKED_ICE
Material PACKED_MUD
Material PAINTING
Material PALE_HANGING_MOSS
Material PALE_MOSS_BLOCK
Material PALE_MOSS_CARPET
Material PALE_OAK_BOAT
Material PALE_OAK_BUTTON
Material PALE_OAK_CHEST_BOAT
Material PALE_OAK_DOOR
Material PALE_OAK_FENCE
Material PALE_OAK_FENCE_GATE
Material PALE_OAK_HANGING_SIGN
Material PALE_OAK_LEAVES
Material PALE_OAK_LOG
Material PALE_OAK_PLANKS
Material PALE_OAK_PRESSURE_PLATE
Material PALE_OAK_SAPLING
Material PALE_OAK_SHELF
Material PALE_OAK_SIGN
Material PALE_OAK_SLAB
Material PALE_OAK_STAIRS
Material PALE_OAK_TRAPDOOR
Material PALE_OAK_WALL_HANGING_SIGN
Material PALE_OAK_WALL_SIGN
Material PALE_OAK_WOOD
Material PANDA_SPAWN_EGG
Material PAPER
Material PARCHED_SPAWN_EGG
Material PARROT_SPAWN_EGG
Material PEARLESCENT_FROGLIGHT
Material PEONY
Material PETRIFIED_OAK_SLAB
Material PHANTOM_MEMBRANE
Material PHANTOM_SPAWN_EGG
Material PIG_SPAWN_EGG
Material PIGLIN_BANNER_PATTERN
Material PIGLIN_BRUTE_SPAWN_EGG
Material PIGLIN_HEAD
Material PIGLIN_SPAWN_EGG
Material PIGLIN_WALL_HEAD
Material PILLAGER_SPAWN_EGG
Material PINK_BANNER
Material PINK_BED
Material PINK_BUNDLE
Material PINK_CANDLE
Material PINK_CANDLE_CAKE
Material PINK_CARPET
Material PINK_CONCRETE
Material PINK_CONCRETE_POWDER
Material PINK_CONCRETE_SLAB
Material PINK_CONCRETE_STAIRS
Material PINK_CUSHION
Material PINK_DYE
Material PINK_GLAZED_TERRACOTTA
Material PINK_HARNESS
Material PINK_PETALS
Material PINK_SHULKER_BOX
Material PINK_STAINED_GLASS
Material PINK_STAINED_GLASS_PANE
Material PINK_TERRACOTTA
Material PINK_TULIP
Material PINK_WALL_BANNER
Material PINK_WOOL
Material PINK_WOOL_SLAB
Material PINK_WOOL_STAIRS
Material PISTON
Material PISTON_HEAD
Material PITCHER_CROP
Material PITCHER_PLANT
Material PITCHER_POD
Material PLAINS_VILLAGE_MAP
Material PLAYER_HEAD
Material PLAYER_WALL_HEAD
Material PLENTY_POTTERY_SHERD
Material PODZOL
Material POINTED_DRIPSTONE
Material POISONOUS_POTATO
Material POLAR_BEAR_SPAWN_EGG
Material POLISHED_ANDESITE
Material POLISHED_ANDESITE_SLAB
Material POLISHED_ANDESITE_STAIRS
Material POLISHED_BASALT
Material POLISHED_BLACKSTONE
Material POLISHED_BLACKSTONE_BRICK_SLAB
Material POLISHED_BLACKSTONE_BRICK_STAIRS
Material POLISHED_BLACKSTONE_BRICK_WALL
Material POLISHED_BLACKSTONE_BRICKS
Material POLISHED_BLACKSTONE_BUTTON
Material POLISHED_BLACKSTONE_PRESSURE_PLATE
Material POLISHED_BLACKSTONE_SLAB
Material POLISHED_BLACKSTONE_STAIRS
Material POLISHED_BLACKSTONE_WALL
Material POLISHED_CINNABAR
Material POLISHED_CINNABAR_SLAB
Material POLISHED_CINNABAR_STAIRS
Material POLISHED_CINNABAR_WALL
Material POLISHED_DEEPSLATE
Material POLISHED_DEEPSLATE_SLAB
Material POLISHED_DEEPSLATE_STAIRS
Material POLISHED_DEEPSLATE_WALL
Material POLISHED_DIORITE
Material POLISHED_DIORITE_SLAB
Material POLISHED_DIORITE_STAIRS
Material POLISHED_GRANITE
Material POLISHED_GRANITE_SLAB
Material POLISHED_GRANITE_STAIRS
Material POLISHED_SULFUR
Material POLISHED_SULFUR_SLAB
Material POLISHED_SULFUR_STAIRS
Material POLISHED_SULFUR_WALL
Material POLISHED_TUFF
Material POLISHED_TUFF_SLAB
Material POLISHED_TUFF_STAIRS
Material POLISHED_TUFF_WALL
Material POPLAR_BOAT
Material POPLAR_BUTTON
Material POPLAR_CHEST_BOAT
Material POPLAR_DOOR
Material POPLAR_FENCE
Material POPLAR_FENCE_GATE
Material POPLAR_HANGING_SIGN
Material POPLAR_LOG
Material POPLAR_PLANKS
Material POPLAR_PRESSURE_PLATE
Material POPLAR_SAPLING
Material POPLAR_SHELF
Material POPLAR_SIGN
Material POPLAR_SLAB
Material POPLAR_STAIRS
Material POPLAR_TRAPDOOR
Material POPLAR_WALL_HANGING_SIGN
Material POPLAR_WALL_SIGN
Material POPLAR_WOOD
Material POPPED_CHORUS_FRUIT
Material POPPY
Material PORKCHOP
Material POTATO
Material POTATOES
Material POTENT_SULFUR
Material POTION
Material POTTED_ACACIA_SAPLING
Material POTTED_ALLIUM
Material POTTED_AZALEA_BUSH
Material POTTED_AZURE_BLUET
Material POTTED_BAMBOO
Material POTTED_BIRCH_SAPLING
Material POTTED_BLUE_ORCHID
Material POTTED_BROWN_MUSHROOM
Material POTTED_CACTUS
Material POTTED_CHERRY_SAPLING
Material POTTED_CLOSED_EYEBLOSSOM
Material POTTED_CORNFLOWER
Material POTTED_CRIMSON_FUNGUS
Material POTTED_CRIMSON_ROOTS
Material POTTED_DANDELION
Material POTTED_DARK_OAK_SAPLING
Material POTTED_DEAD_BUSH
Material POTTED_FERN
Material POTTED_FLOWERING_AZALEA_BUSH
Material POTTED_GOLDEN_DANDELION
Material POTTED_JUNGLE_SAPLING
Material POTTED_LILY_OF_THE_VALLEY
Material POTTED_MANGROVE_PROPAGULE
Material POTTED_OAK_SAPLING
Material POTTED_OPEN_EYEBLOSSOM
Material POTTED_ORANGE_TULIP
Material POTTED_OXEYE_DAISY
Material POTTED_PALE_OAK_SAPLING
Material POTTED_PINK_TULIP
Material POTTED_POPLAR_SAPLING
Material POTTED_POPPY
Material POTTED_RED_MUSHROOM
Material POTTED_RED_TULIP
Material POTTED_SPRUCE_SAPLING
Material POTTED_TORCHFLOWER
Material POTTED_WARPED_FUNGUS
Material POTTED_WARPED_ROOTS
Material POTTED_WHITE_TULIP
Material POTTED_WITHER_ROSE
Material POWDER_SNOW
Material POWDER_SNOW_BUCKET
Material POWDER_SNOW_CAULDRON
Material POWERED_RAIL
Material PRISMARINE
Material PRISMARINE_BRICK_SLAB
Material PRISMARINE_BRICK_STAIRS
Material PRISMARINE_BRICKS
Material PRISMARINE_CRYSTALS
Material PRISMARINE_SHARD
Material PRISMARINE_SLAB
Material PRISMARINE_STAIRS
Material PRISMARINE_WALL
Material PRIZE_POTTERY_SHERD
Material PUFFERFISH
Material PUFFERFISH_BUCKET
Material PUFFERFISH_SPAWN_EGG
Material PUMPKIN
Material PUMPKIN_PIE
Material PUMPKIN_SEEDS
Material PUMPKIN_STEM
Material PURPLE_BANNER
Material PURPLE_BED
Material PURPLE_BUNDLE
Material PURPLE_CANDLE
Material PURPLE_CANDLE_CAKE
Material PURPLE_CARPET
Material PURPLE_CONCRETE
Material PURPLE_CONCRETE_POWDER
Material PURPLE_CONCRETE_SLAB
Material PURPLE_CONCRETE_STAIRS
Material PURPLE_CUSHION
Material PURPLE_DYE
Material PURPLE_GLAZED_TERRACOTTA
Material PURPLE_HARNESS
Material PURPLE_SHULKER_BOX
Material PURPLE_STAINED_GLASS
Material PURPLE_STAINED_GLASS_PANE
Material PURPLE_TERRACOTTA
Material PURPLE_WALL_BANNER
Material PURPLE_WOOL
Material PURPLE_WOOL_SLAB
Material PURPLE_WOOL_STAIRS
Material PURPUR_BLOCK
Material PURPUR_PILLAR
Material PURPUR_SLAB
Material PURPUR_STAIRS
Material QUARTZ
Material QUARTZ_BLOCK
Material QUARTZ_BRICKS
Material QUARTZ_PILLAR
Material QUARTZ_SLAB
Material QUARTZ_STAIRS
Material RABBIT
Material RABBIT_FOOT
Material RABBIT_HIDE
Material RABBIT_SPAWN_EGG
Material RABBIT_STEW
Material RAIL
Material RAISER_ARMOR_TRIM_SMITHING_TEMPLATE
Material RAVAGER_SPAWN_EGG
Material RAW_COPPER
Material RAW_COPPER_BLOCK
Material RAW_GOLD
Material RAW_GOLD_BLOCK
Material RAW_IRON
Material RAW_IRON_BLOCK
Material RECOVERY_COMPASS
Material RED_BANNER
Material RED_BED
Material RED_BUNDLE
Material RED_CANDLE
Material RED_CANDLE_CAKE
Material RED_CARPET
Material RED_CONCRETE
Material RED_CONCRETE_POWDER
Material RED_CONCRETE_SLAB
Material RED_CONCRETE_STAIRS
Material RED_CUSHION
Material RED_DYE
Material RED_GLAZED_TERRACOTTA
Material RED_HARNESS
Material RED_MUSHROOM
Material RED_MUSHROOM_BLOCK
Material RED_NETHER_BRICK_SLAB
Material RED_NETHER_BRICK_STAIRS
Material RED_NETHER_BRICK_WALL
Material RED_NETHER_BRICKS
Material RED_POPLAR_LEAVES
Material RED_SAND
Material RED_SANDSTONE
Material RED_SANDSTONE_SLAB
Material RED_SANDSTONE_STAIRS
Material RED_SANDSTONE_WALL
Material RED_SHRUB
Material RED_SHULKER_BOX
Material RED_STAINED_GLASS
Material RED_STAINED_GLASS_PANE
Material RED_TERRACOTTA
Material RED_TULIP
Material RED_WALL_BANNER
Material RED_WOOL
Material RED_WOOL_SLAB
Material RED_WOOL_STAIRS
Material REDSTONE
Material REDSTONE_BLOCK
Material REDSTONE_LAMP
Material REDSTONE_ORE
Material REDSTONE_TORCH
Material REDSTONE_WALL_TORCH
Material REDSTONE_WIRE
Material REINFORCED_DEEPSLATE
Material REPEATER
Material REPEATING_COMMAND_BLOCK
Material RESIN_BLOCK
Material RESIN_BRICK
Material RESIN_BRICK_SLAB
Material RESIN_BRICK_STAIRS
Material RESIN_BRICK_WALL
Material RESIN_BRICKS
Material RESIN_CLUMP
Material RESPAWN_ANCHOR
Material RIB_ARMOR_TRIM_SMITHING_TEMPLATE
Material ROOTED_DIRT
Material ROSE_BUSH
Material ROTTEN_FLESH
Material SADDLE
Material SALMON
Material SALMON_BUCKET
Material SALMON_SPAWN_EGG
Material SAND
Material SANDSTONE
Material SANDSTONE_SLAB
Material SANDSTONE_STAIRS
Material SANDSTONE_WALL
Material SAVANNA_VILLAGE_MAP
Material SCAFFOLDING
Material SCRAPE_POTTERY_SHERD
Material SCULK
Material SCULK_CATALYST
Material SCULK_SENSOR
Material SCULK_SHRIEKER
Material SCULK_VEIN
Material SEA_LANTERN
Material SEA_PICKLE
Material SEAGRASS
Material SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE
Material SHAPER_ARMOR_TRIM_SMITHING_TEMPLATE
Material SHEAF_POTTERY_SHERD
Material SHEARS
Material SHEEP_SPAWN_EGG
Material SHELF_MUSHROOM
Material SHELTER_POTTERY_SHERD
Material SHIELD
Material SHORT_DRY_GRASS
Material SHORT_GRASS
Material SHROOMLIGHT
Material SHULKER_BOX
Material SHULKER_SHELL
Material SHULKER_SPAWN_EGG
Material SILENCE_ARMOR_TRIM_SMITHING_TEMPLATE
Material SILVERFISH_SPAWN_EGG
Material SKELETON_HORSE_SPAWN_EGG
Material SKELETON_SKULL
Material SKELETON_SPAWN_EGG
Material SKELETON_WALL_SKULL
Material SKULL_BANNER_PATTERN
Material SKULL_POTTERY_SHERD
Material SLIME_BALL
Material SLIME_BLOCK
Material SLIME_SPAWN_EGG
Material SMALL_AMETHYST_BUD
Material SMALL_DRIPLEAF
Material SMITHING_TABLE
Material SMOKER
Material SMOOTH_BASALT
Material SMOOTH_QUARTZ
Material SMOOTH_QUARTZ_SLAB
Material SMOOTH_QUARTZ_STAIRS
Material SMOOTH_RED_SANDSTONE
Material SMOOTH_RED_SANDSTONE_SLAB
Material SMOOTH_RED_SANDSTONE_STAIRS
Material SMOOTH_SANDSTONE
Material SMOOTH_SANDSTONE_SLAB
Material SMOOTH_SANDSTONE_STAIRS
Material SMOOTH_STONE
Material SMOOTH_STONE_SLAB
Material SNIFFER_EGG
Material SNIFFER_SPAWN_EGG
Material SNORT_POTTERY_SHERD
Material SNOUT_ARMOR_TRIM_SMITHING_TEMPLATE
Material SNOW
Material SNOW_BLOCK
Material SNOW_GOLEM_SPAWN_EGG
Material SNOWBALL
Material SNOWY_VILLAGE_MAP
Material SOUL_CAMPFIRE
Material SOUL_FIRE
Material SOUL_LANTERN
Material SOUL_SAND
Material SOUL_SOIL
Material SOUL_TORCH
Material SOUL_WALL_TORCH
Material SPAWNER
Material SPECTRAL_ARROW
Material SPIDER_EYE
Material SPIDER_SPAWN_EGG
Material SPIRE_ARMOR_TRIM_SMITHING_TEMPLATE
Material SPLASH_POTION
Material SPONGE
Material SPORE_BLOSSOM
Material SPRUCE_BOAT
Material SPRUCE_BUTTON
Material SPRUCE_CHEST_BOAT
Material SPRUCE_DOOR
Material SPRUCE_FENCE
Material SPRUCE_FENCE_GATE
Material SPRUCE_HANGING_SIGN
Material SPRUCE_LEAVES
Material SPRUCE_LOG
Material SPRUCE_PLANKS
Material SPRUCE_PRESSURE_PLATE
Material SPRUCE_SAPLING
Material SPRUCE_SHELF
Material SPRUCE_SIGN
Material SPRUCE_SLAB
Material SPRUCE_STAIRS
Material SPRUCE_TRAPDOOR
Material SPRUCE_WALL_HANGING_SIGN
Material SPRUCE_WALL_SIGN
Material SPRUCE_WOOD
Material SPYGLASS
Material SQUID_SPAWN_EGG
Material STICK
Material STICKY_PISTON
Material STONE
Material STONE_AXE
Material STONE_BRICK_SLAB
Material STONE_BRICK_STAIRS
Material STONE_BRICK_WALL
Material STONE_BRICKS
Material STONE_BUTTON
Material STONE_HOE
Material STONE_PICKAXE
Material STONE_PRESSURE_PLATE
Material STONE_SHOVEL
Material STONE_SLAB
Material STONE_SPEAR
Material STONE_STAIRS
Material STONE_SWORD
Material STONECUTTER
Material STRAW_BED
Material STRAY_SPAWN_EGG
Material STRIDER_SPAWN_EGG
Material STRING
Material STRIPPED_ACACIA_LOG
Material STRIPPED_ACACIA_WOOD
Material STRIPPED_BAMBOO_BLOCK
Material STRIPPED_BIRCH_LOG
Material STRIPPED_BIRCH_WOOD
Material STRIPPED_CHERRY_LOG
Material STRIPPED_CHERRY_WOOD
Material STRIPPED_CRIMSON_HYPHAE
Material STRIPPED_CRIMSON_STEM
Material STRIPPED_DARK_OAK_LOG
Material STRIPPED_DARK_OAK_WOOD
Material STRIPPED_JUNGLE_LOG
Material STRIPPED_JUNGLE_WOOD
Material STRIPPED_MANGROVE_LOG
Material STRIPPED_MANGROVE_WOOD
Material STRIPPED_OAK_LOG
Material STRIPPED_OAK_WOOD
Material STRIPPED_PALE_OAK_LOG
Material STRIPPED_PALE_OAK_WOOD
Material STRIPPED_POPLAR_LOG
Material STRIPPED_POPLAR_WOOD
Material STRIPPED_SPRUCE_LOG
Material STRIPPED_SPRUCE_WOOD
Material STRIPPED_WARPED_HYPHAE
Material STRIPPED_WARPED_STEM
Material STRUCTURE_BLOCK
Material STRUCTURE_VOID
Material SUGAR
Material SUGAR_CANE
Material SULFUR
Material SULFUR_BRICK_SLAB
Material SULFUR_BRICK_STAIRS
Material SULFUR_BRICK_WALL
Material SULFUR_BRICKS
Material SULFUR_CUBE_BUCKET
Material SULFUR_CUBE_SPAWN_EGG
Material SULFUR_SLAB
Material SULFUR_SPIKE
Material SULFUR_STAIRS
Material SULFUR_WALL
Material SUNFLOWER
Material SUSPICIOUS_GRAVEL
Material SUSPICIOUS_SAND
Material SUSPICIOUS_STEW
Material SWAMP_HUT_MAP
Material SWEET_BERRIES
Material SWEET_BERRY_BUSH
Material TADPOLE_BUCKET
Material TADPOLE_SPAWN_EGG
Material TAIGA_VILLAGE_MAP
Material TALL_DRY_GRASS
Material TALL_GRASS
Material TALL_SEAGRASS
Material TARGET
Material TERRACOTTA
Material TEST_BLOCK
Material TEST_INSTANCE_BLOCK
Material TIDE_ARMOR_TRIM_SMITHING_TEMPLATE
Material TINTED_GLASS
Material TIPPED_ARROW
Material TNT
Material TNT_MINECART
Material TORCH
Material TORCHFLOWER
Material TORCHFLOWER_CROP
Material TORCHFLOWER_SEEDS
Material TOTEM_OF_UNDYING
Material TRADER_LLAMA_SPAWN_EGG
Material TRAPPED_CHEST
Material TRIAL_KEY
Material TRIAL_SPAWNER
Material TRIDENT
Material TRIPWIRE
Material TRIPWIRE_HOOK
Material TROPICAL_FISH
Material TROPICAL_FISH_BUCKET
Material TROPICAL_FISH_SPAWN_EGG
Material TUBE_CORAL
Material TUBE_CORAL_BLOCK
Material TUBE_CORAL_FAN
Material TUBE_CORAL_WALL_FAN
Material TUFF
Material TUFF_BRICK_SLAB
Material TUFF_BRICK_STAIRS
Material TUFF_BRICK_WALL
Material TUFF_BRICKS
Material TUFF_SLAB
Material TUFF_STAIRS
Material TUFF_WALL
Material TURTLE_EGG
Material TURTLE_HELMET
Material TURTLE_SCUTE
Material TURTLE_SPAWN_EGG
Material TWISTING_VINES
Material TWISTING_VINES_PLANT
Material VAULT
Material VERDANT_FROGLIGHT
Material VEX_ARMOR_TRIM_SMITHING_TEMPLATE
Material VEX_SPAWN_EGG
Material VILLAGER_SPAWN_EGG
Material VINDICATOR_SPAWN_EGG
Material VINE
Material VOID_AIR
Material WALL_TORCH
Material WANDERING_TRADER_SPAWN_EGG
Material WARD_ARMOR_TRIM_SMITHING_TEMPLATE
Material WARDEN_SPAWN_EGG
Material WARM_OCEAN_RUINS_MAP
Material WARPED_BUTTON
Material WARPED_DOOR
Material WARPED_FENCE
Material WARPED_FENCE_GATE
Material WARPED_FUNGUS
Material WARPED_FUNGUS_ON_A_STICK
Material WARPED_HANGING_SIGN
Material WARPED_HYPHAE
Material WARPED_NYLIUM
Material WARPED_PLANKS
Material WARPED_PRESSURE_PLATE
Material WARPED_ROOTS
Material WARPED_SHELF
Material WARPED_SIGN
Material WARPED_SLAB
Material WARPED_STAIRS
Material WARPED_STEM
Material WARPED_TRAPDOOR
Material WARPED_WALL_HANGING_SIGN
Material WARPED_WALL_SIGN
Material WARPED_WART_BLOCK
Material WATER
Material WATER_BUCKET
Material WATER_CAULDRON
Material WAXED_CHISELED_COPPER
Material WAXED_COPPER_BARS
Material WAXED_COPPER_BLOCK
Material WAXED_COPPER_BULB
Material WAXED_COPPER_CHAIN
Material WAXED_COPPER_CHEST
Material WAXED_COPPER_DOOR
Material WAXED_COPPER_GOLEM_STATUE
Material WAXED_COPPER_GRATE
Material WAXED_COPPER_LANTERN
Material WAXED_COPPER_TRAPDOOR
Material WAXED_CUT_COPPER
Material WAXED_CUT_COPPER_SLAB
Material WAXED_CUT_COPPER_STAIRS
Material WAXED_EXPOSED_CHISELED_COPPER
Material WAXED_EXPOSED_COPPER
Material WAXED_EXPOSED_COPPER_BARS
Material WAXED_EXPOSED_COPPER_BULB
Material WAXED_EXPOSED_COPPER_CHAIN
Material WAXED_EXPOSED_COPPER_CHEST
Material WAXED_EXPOSED_COPPER_DOOR
Material WAXED_EXPOSED_COPPER_GOLEM_STATUE
Material WAXED_EXPOSED_COPPER_GRATE
Material WAXED_EXPOSED_COPPER_LANTERN
Material WAXED_EXPOSED_COPPER_TRAPDOOR
Material WAXED_EXPOSED_CUT_COPPER
Material WAXED_EXPOSED_CUT_COPPER_SLAB
Material WAXED_EXPOSED_CUT_COPPER_STAIRS
Material WAXED_EXPOSED_LIGHTNING_ROD
Material WAXED_LIGHTNING_ROD
Material WAXED_OXIDIZED_CHISELED_COPPER
Material WAXED_OXIDIZED_COPPER
Material WAXED_OXIDIZED_COPPER_BARS
Material WAXED_OXIDIZED_COPPER_BULB
Material WAXED_OXIDIZED_COPPER_CHAIN
Material WAXED_OXIDIZED_COPPER_CHEST
Material WAXED_OXIDIZED_COPPER_DOOR
Material WAXED_OXIDIZED_COPPER_GOLEM_STATUE
Material WAXED_OXIDIZED_COPPER_GRATE
Material WAXED_OXIDIZED_COPPER_LANTERN
Material WAXED_OXIDIZED_COPPER_TRAPDOOR
Material WAXED_OXIDIZED_CUT_COPPER
Material WAXED_OXIDIZED_CUT_COPPER_SLAB
Material WAXED_OXIDIZED_CUT_COPPER_STAIRS
Material WAXED_OXIDIZED_LIGHTNING_ROD
Material WAXED_WEATHERED_CHISELED_COPPER
Material WAXED_WEATHERED_COPPER
Material WAXED_WEATHERED_COPPER_BARS
Material WAXED_WEATHERED_COPPER_BULB
Material WAXED_WEATHERED_COPPER_CHAIN
Material WAXED_WEATHERED_COPPER_CHEST
Material WAXED_WEATHERED_COPPER_DOOR
Material WAXED_WEATHERED_COPPER_GOLEM_STATUE
Material WAXED_WEATHERED_COPPER_GRATE
Material WAXED_WEATHERED_COPPER_LANTERN
Material WAXED_WEATHERED_COPPER_TRAPDOOR
Material WAXED_WEATHERED_CUT_COPPER
Material WAXED_WEATHERED_CUT_COPPER_SLAB
Material WAXED_WEATHERED_CUT_COPPER_STAIRS
Material WAXED_WEATHERED_LIGHTNING_ROD
Material WAYFINDER_ARMOR_TRIM_SMITHING_TEMPLATE
Material WEATHERED_CHISELED_COPPER
Material WEATHERED_COPPER
Material WEATHERED_COPPER_BARS
Material WEATHERED_COPPER_BULB
Material WEATHERED_COPPER_CHAIN
Material WEATHERED_COPPER_CHEST
Material WEATHERED_COPPER_DOOR
Material WEATHERED_COPPER_GOLEM_STATUE
Material WEATHERED_COPPER_GRATE
Material WEATHERED_COPPER_LANTERN
Material WEATHERED_COPPER_TRAPDOOR
Material WEATHERED_CUT_COPPER
Material WEATHERED_CUT_COPPER_SLAB
Material WEATHERED_CUT_COPPER_STAIRS
Material WEATHERED_LIGHTNING_ROD
Material WEEPING_VINES
Material WEEPING_VINES_PLANT
Material WET_SPONGE
Material WHEAT
Material WHEAT_SEEDS
Material WHITE_BANNER
Material WHITE_BED
Material WHITE_BUNDLE
Material WHITE_CANDLE
Material WHITE_CANDLE_CAKE
Material WHITE_CARPET
Material WHITE_CONCRETE
Material WHITE_CONCRETE_POWDER
Material WHITE_CONCRETE_SLAB
Material WHITE_CONCRETE_STAIRS
Material WHITE_CUSHION
Material WHITE_DYE
Material WHITE_GLAZED_TERRACOTTA
Material WHITE_HARNESS
Material WHITE_SHULKER_BOX
Material WHITE_STAINED_GLASS
Material WHITE_STAINED_GLASS_PANE
Material WHITE_TERRACOTTA
Material WHITE_TULIP
Material WHITE_WALL_BANNER
Material WHITE_WOOL
Material WHITE_WOOL_SLAB
Material WHITE_WOOL_STAIRS
Material WILD_ARMOR_TRIM_SMITHING_TEMPLATE
Material WILDFLOWERS
Material WIND_CHARGE
Material WITCH_SPAWN_EGG
Material WITHER_ROSE
Material WITHER_SKELETON_SKULL
Material WITHER_SKELETON_SPAWN_EGG
Material WITHER_SKELETON_WALL_SKULL
Material WITHER_SPAWN_EGG
Material WOLF_ARMOR
Material WOLF_SPAWN_EGG
Material WOODEN_AXE
Material WOODEN_HOE
Material WOODEN_PICKAXE
Material WOODEN_SHOVEL
Material WOODEN_SPEAR
Material WOODEN_SWORD
Material WOODLAND_MANSION_MAP
Material WRITABLE_BOOK
Material WRITTEN_BOOK
Material YELLOW_BANNER
Material YELLOW_BED
Material YELLOW_BUNDLE
Material YELLOW_CANDLE
Material YELLOW_CANDLE_CAKE
Material YELLOW_CARPET
Material YELLOW_CONCRETE
Material YELLOW_CONCRETE_POWDER
Material YELLOW_CONCRETE_SLAB
Material YELLOW_CONCRETE_STAIRS
Material YELLOW_CUSHION
Material YELLOW_DYE
Material YELLOW_GLAZED_TERRACOTTA
Material YELLOW_HARNESS
Material YELLOW_POPLAR_LEAVES
Material YELLOW_SHULKER_BOX
Material YELLOW_STAINED_GLASS
Material YELLOW_STAINED_GLASS_PANE
Material YELLOW_TERRACOTTA
Material YELLOW_WALL_BANNER
Material YELLOW_WOOL
Material YELLOW_WOOL_SLAB
Material YELLOW_WOOL_STAIRS
Material ZOGLIN_SPAWN_EGG
Material ZOMBIE_HEAD
Material ZOMBIE_HORSE_SPAWN_EGG
Material ZOMBIE_NAUTILUS_SPAWN_EGG
Material ZOMBIE_SPAWN_EGG
Material ZOMBIE_VILLAGER_SPAWN_EGG
Material ZOMBIE_WALL_HEAD
Material ZOMBIFIED_PIGLIN_SPAWN_EGG
Material[] values()
Material valueOf(String)
NamespacedKey getKey()
NamespacedKey key()
boolean isBlock()
boolean isItem()
boolean isSolid()
boolean isEdible()
boolean isAir()
int getMaxStackSize()
boolean isLegacy()
String translationKey()
Material fromKey(String)
Material getMaterial(String)
Material matchMaterial(String)
```

### NamespacedKey (class)

```java
String MINECRAFT
NamespacedKey(String, String)
NamespacedKey(Plugin, String)
NamespacedKey minecraft(String)
NamespacedKey fromString(String)
String getNamespace()
String getKey()
String asString()
String toString()
boolean equals(Object)
int hashCode()
```

### OfflinePlayer (interface)

```java
boolean isOnline()
String getName()
UUID getUniqueId()
Player getPlayer()
boolean hasPlayedBefore()
```

### Particle (class)

```java
Particle ANGRY_VILLAGER
Particle ASH
Particle BLOCK
Particle BLOCK_MARKER
Particle BUBBLE
Particle BUBBLE_COLUMN_UP
Particle BUBBLE_POP
Particle CAMPFIRE_COSY_SMOKE
Particle CAMPFIRE_SIGNAL_SMOKE
Particle CHERRY_LEAVES
Particle CLOUD
Particle COMPOSTER
Particle CRIMSON_SPORE
Particle CRIT
Particle CURRENT_DOWN
Particle DAMAGE_INDICATOR
Particle DOLPHIN
Particle DRAGON_BREATH
Particle DRIPPING_DRIPSTONE_LAVA
Particle DRIPPING_DRIPSTONE_WATER
Particle DRIPPING_HONEY
Particle DRIPPING_LAVA
Particle DRIPPING_OBSIDIAN_TEAR
Particle DRIPPING_WATER
Particle DUST
Particle DUST_COLOR_TRANSITION
Particle DUST_PILLAR
Particle DUST_PLUME
Particle EFFECT
Particle EGG_CRACK
Particle ELDER_GUARDIAN
Particle ELECTRIC_SPARK
Particle ENCHANT
Particle ENCHANTED_HIT
Particle END_ROD
Particle ENTITY_EFFECT
Particle EXPLOSION
Particle EXPLOSION_EMITTER
Particle FALLING_DRIPSTONE_LAVA
Particle FALLING_DRIPSTONE_WATER
Particle FALLING_DUST
Particle FALLING_HONEY
Particle FALLING_LAVA
Particle FALLING_NECTAR
Particle FALLING_OBSIDIAN_TEAR
Particle FALLING_SPORE_BLOSSOM
Particle FALLING_WATER
Particle FIREWORK
Particle FISHING
Particle FLAME
Particle FLASH
Particle GLOW
Particle GLOW_SQUID_INK
Particle GUST
Particle GUST_EMITTER_LARGE
Particle GUST_EMITTER_SMALL
Particle HAPPY_VILLAGER
Particle HEART
Particle INFESTED
Particle INSTANT_EFFECT
Particle ITEM
Particle ITEM_COBWEB
Particle ITEM_SLIME
Particle ITEM_SNOWBALL
Particle LANDING_HONEY
Particle LANDING_LAVA
Particle LANDING_OBSIDIAN_TEAR
Particle LARGE_SMOKE
Particle LAVA
Particle MYCELIUM
Particle NAUTILUS
Particle NOTE
Particle OMINOUS_SPAWNING
Particle POOF
Particle PORTAL
Particle RAID_OMEN
Particle RAIN
Particle REVERSE_PORTAL
Particle SCRAPE
Particle SCULK_CHARGE
Particle SCULK_CHARGE_POP
Particle SCULK_SOUL
Particle SHRIEK
Particle SMALL_FLAME
Particle SMALL_GUST
Particle SMOKE
Particle SNEEZE
Particle SNOWFLAKE
Particle SONIC_BOOM
Particle SOUL
Particle SOUL_FIRE_FLAME
Particle SPIT
Particle SPLASH
Particle SPORE_BLOSSOM_AIR
Particle SQUID_INK
Particle SWEEP_ATTACK
Particle TOTEM_OF_UNDYING
Particle TRIAL_OMEN
Particle TRIAL_SPAWNER_DETECTION
Particle TRIAL_SPAWNER_DETECTION_OMINOUS
Particle UNDERWATER
Particle VAULT_CONNECTION
Particle VIBRATION
Particle WARPED_SPORE
Particle WAX_OFF
Particle WAX_ON
Particle WHITE_ASH
Particle WHITE_SMOKE
Particle WITCH
Particle BLOCK_CRUMBLE
Particle COPPER_FIRE_FLAME
Particle FIREFLY
Particle GEYSER
Particle GEYSER_BASE
Particle GEYSER_PLUME
Particle GEYSER_POOF
Particle NOXIOUS_GAS
Particle NOXIOUS_GAS_CLOUD
Particle ORANGE_POPLAR_LEAVES
Particle PALE_OAK_LEAVES
Particle PAUSE_MOB_GROWTH
Particle RED_POPLAR_LEAVES
Particle RESET_MOB_GROWTH
Particle SULFUR_BUBBLES
Particle SULFUR_CUBE_GOO
Particle TINTED_LEAVES
Particle TRAIL
Particle YELLOW_POPLAR_LEAVES
Particle[] values()
Particle valueOf(String)
NamespacedKey getKey()
NamespacedKey key()
Class<?> getDataType()
```

### Server (interface)

```java
String getName()
String getVersion()
String getBukkitVersion()
String getMinecraftVersion()
Collection<? extends Player> getOnlinePlayers()
int getMaxPlayers()
int getPort()
String getIp()
String getMotd()
boolean getOnlineMode()
int getViewDistance()
GameMode getDefaultGameMode()
Player getPlayer(String)
Player getPlayerExact(String)
Player getPlayer(UUID)
List<Player> matchPlayer(String)
OfflinePlayer getOfflinePlayer(String)
OfflinePlayer getOfflinePlayer(UUID)
PluginManager getPluginManager()
BukkitScheduler getScheduler()
World getWorld(String)
World getWorld(UUID)
List<World> getWorlds()
int broadcastMessage(String)
int broadcast(String, String)
int broadcast(Component)
ConsoleCommandSender getConsoleSender()
boolean dispatchCommand(CommandSender, String)
PluginCommand getPluginCommand(String)
logging.Logger getLogger()
boolean isPrimaryThread()
void shutdown()
double[] getTPS()
void reload()
void savePlayers()
Inventory createInventory(InventoryHolder, int)
Inventory createInventory(InventoryHolder, int, String)
Inventory createInventory(InventoryHolder, int, Component)
Inventory createInventory(InventoryHolder, InventoryType)
Inventory createInventory(InventoryHolder, InventoryType, String)
Inventory createInventory(InventoryHolder, InventoryType, Component)
BossBar createBossBar(String, BarColor, BarStyle, ..)
ScoreboardManager getScoreboardManager()
ItemFactory getItemFactory()
```

### World (interface)

```java
String getName()
UUID getUID()
World$Environment getEnvironment()
Block getBlockAt(int, int, int)
Block getBlockAt(Location)
int getHighestBlockYAt(int, int)
Block getHighestBlockAt(int, int)
Location getSpawnLocation()
boolean setSpawnLocation(int, int, int)
long getTime()
void setTime(long)
long getFullTime()
void setFullTime(long)
List<Player> getPlayers()
Item dropItem(Location, ItemStack)
Item dropItemNaturally(Location, ItemStack)
int getMinHeight()
int getMaxHeight()
boolean isDayTime()
void playSound(Location, String, float, float)
void createExplosion(Location, float)
boolean createExplosion(double, double, double, float)
boolean createExplosion(double, double, double, float, boolean, boolean)
boolean createExplosion(Location, float, boolean, boolean)
List<Entity> getEntities()
List<LivingEntity> getLivingEntities()
Collection<Entity> getNearbyEntities(Location, double, double, double)
Entity spawnEntity(Location, EntityType)
void strikeLightningEffect(Location)
<T> T getGameRuleValue(GameRule<T>)
<T> boolean setGameRule(GameRule<T>, T)
<T> T getGameRuleDefault(GameRule<T>)
String getGameRuleValue(String)
boolean setGameRuleValue(String, String)
boolean isGameRule(String)
String[] getGameRules()
<T> void spawnParticle(Particle, double, double, double, int, double, double, double, double, T, boolean)
void spawnParticle(Particle, Location, int)
void spawnParticle(Particle, double, double, double, int)
<T> void spawnParticle(Particle, Location, int, T)
void spawnParticle(Particle, Location, int, double, double, double)
void spawnParticle(Particle, Location, int, double, double, double, double)
<T> void spawnParticle(Particle, Location, int, double, double, double, T)
<T> void spawnParticle(Particle, Location, int, double, double, double, double, T)
<T> void spawnParticle(Particle, Location, int, double, double, double, double, T, boolean)
```

## `org.bukkit.block`

### Block (interface)

```java
Material getType()
void setType(Material)
void setType(Material, boolean)
int getX()
int getY()
int getZ()
World getWorld()
Location getLocation()
Block getRelative(int, int, int)
Block getRelative(BlockFace)
Block getRelative(BlockFace, int)
boolean isEmpty()
boolean isLiquid()
boolean isSolid()
boolean breakNaturally()
String getBlockDataString()
```

### BlockFace (class)

```java
BlockFace NORTH
BlockFace EAST
BlockFace SOUTH
BlockFace WEST
BlockFace UP
BlockFace DOWN
BlockFace NORTH_EAST
BlockFace NORTH_WEST
BlockFace SOUTH_EAST
BlockFace SOUTH_WEST
BlockFace SELF
BlockFace[] values()
BlockFace valueOf(String)
int getModX()
int getModY()
int getModZ()
Vector getDirection()
BlockFace getOppositeFace()
BlockFace fromProtocol(int)
```

## `org.bukkit.boss`

### BarColor (class)

```java
BarColor PINK
BarColor BLUE
BarColor RED
BarColor GREEN
BarColor YELLOW
BarColor PURPLE
BarColor WHITE
BarColor[] values()
BarColor valueOf(String)
```

### BarFlag (class)

```java
BarFlag DARKEN_SKY
BarFlag PLAY_BOSS_MUSIC
BarFlag CREATE_FOG
BarFlag[] values()
BarFlag valueOf(String)
```

### BarStyle (class)

```java
BarStyle SOLID
BarStyle SEGMENTED_6
BarStyle SEGMENTED_10
BarStyle SEGMENTED_12
BarStyle SEGMENTED_20
BarStyle[] values()
BarStyle valueOf(String)
```

### BossBar (interface)

```java
String getTitle()
void setTitle(String)
BarColor getColor()
void setColor(BarColor)
BarStyle getStyle()
void setStyle(BarStyle)
void removeFlag(BarFlag)
void addFlag(BarFlag)
boolean hasFlag(BarFlag)
void setProgress(double)
double getProgress()
void addPlayer(Player)
void removePlayer(Player)
void removeAll()
List<Player> getPlayers()
void setVisible(boolean)
boolean isVisible()
```

### KeyedBossBar (interface)

```java
NamespacedKey getKey()
```

## `org.bukkit.command`

### Command (class)

```java
boolean execute(CommandSender, String, String[])
List<String> tabComplete(CommandSender, String, String[])
String getName()
boolean setName(String)
String getLabel()
boolean setLabel(String)
List<String> getAliases()
Command setAliases(List<String>)
String getDescription()
Command setDescription(String)
String getUsage()
Command setUsage(String)
String getPermission()
void setPermission(String)
String getPermissionMessage()
Command setPermissionMessage(String)
boolean testPermissionSilent(CommandSender)
boolean testPermission(CommandSender)
String toString()
```

### CommandException (class)

```java
CommandException()
CommandException(String)
CommandException(String, Throwable)
```

### CommandExecutor (interface)

```java
boolean onCommand(CommandSender, Command, String, String[])
```

### CommandSender (interface)

```java
void sendMessage(String)
void sendMessage(String...)
Server getServer()
String getName()
Component name()
void sendLegacy(String)
```

### ConsoleCommandSender (interface)

### PluginCommand (class)

```java
PluginCommand(String, Plugin)
boolean execute(CommandSender, String, String[])
List<String> tabComplete(CommandSender, String, String[])
void setExecutor(CommandExecutor)
CommandExecutor getExecutor()
void setTabCompleter(TabCompleter)
TabCompleter getTabCompleter()
Plugin getPlugin()
```

### PluginIdentifiableCommand (interface)

```java
Plugin getPlugin()
```

### TabCompleter (interface)

```java
List<String> onTabComplete(CommandSender, Command, String, String[])
```

### TabExecutor (interface)

## `org.bukkit.configuration`

### Configuration (interface)

```java
void addDefaults(Map<String, Object>)
void addDefaults(Configuration)
void setDefaults(Configuration)
Configuration getDefaults()
```

### ConfigurationSection (interface)

```java
Set<String> getKeys(boolean)
Map<String, Object> getValues(boolean)
boolean contains(String)
boolean contains(String, boolean)
boolean isSet(String)
String getCurrentPath()
String getName()
Configuration getRoot()
ConfigurationSection getParent()
Object get(String)
Object get(String, Object)
void set(String, Object)
ConfigurationSection createSection(String)
ConfigurationSection createSection(String, Map<?, ?>)
String getString(String)
String getString(String, String)
boolean isString(String)
int getInt(String)
int getInt(String, int)
boolean isInt(String)
boolean getBoolean(String)
boolean getBoolean(String, boolean)
boolean isBoolean(String)
double getDouble(String)
double getDouble(String, double)
boolean isDouble(String)
long getLong(String)
long getLong(String, long)
boolean isLong(String)
List<?> getList(String)
List<?> getList(String, List<?>)
boolean isList(String)
List<String> getStringList(String)
List<Integer> getIntegerList(String)
List<Double> getDoubleList(String)
List<Map<?, ?>> getMapList(String)
ConfigurationSection getConfigurationSection(String)
boolean isConfigurationSection(String)
ConfigurationSection getDefaultSection()
void addDefault(String, Object)
```

### InvalidConfigurationException (class)

```java
InvalidConfigurationException()
InvalidConfigurationException(String)
InvalidConfigurationException(Throwable)
InvalidConfigurationException(String, Throwable)
```

### MemoryConfiguration (class)

```java
MemoryConfiguration()
MemoryConfiguration(Configuration)
void addDefault(String, Object)
void addDefaults(Map<String, Object>)
void addDefaults(Configuration)
void setDefaults(Configuration)
Configuration getDefaults()
ConfigurationSection getParent()
```

### MemorySection (class)

```java
Set<String> getKeys(boolean)
Map<String, Object> getValues(boolean)
boolean contains(String)
boolean contains(String, boolean)
boolean isSet(String)
String getCurrentPath()
String getName()
Configuration getRoot()
ConfigurationSection getParent()
Object get(String)
Object get(String, Object)
void set(String, Object)
ConfigurationSection createSection(String)
ConfigurationSection createSection(String, Map<?, ?>)
Map<String, Object> toMap()
String getString(String)
String getString(String, String)
boolean isString(String)
int getInt(String)
int getInt(String, int)
boolean isInt(String)
boolean getBoolean(String)
boolean getBoolean(String, boolean)
boolean isBoolean(String)
double getDouble(String)
double getDouble(String, double)
boolean isDouble(String)
long getLong(String)
long getLong(String, long)
boolean isLong(String)
List<?> getList(String)
List<?> getList(String, List<?>)
boolean isList(String)
List<String> getStringList(String)
List<Integer> getIntegerList(String)
List<Double> getDoubleList(String)
List<Map<?, ?>> getMapList(String)
ConfigurationSection getConfigurationSection(String)
boolean isConfigurationSection(String)
ConfigurationSection getDefaultSection()
void addDefault(String, Object)
String toString()
```

## `org.bukkit.configuration.file`

### FileConfiguration (class)

```java
FileConfiguration()
FileConfiguration(Configuration)
FileConfigurationOptions options()
String saveToString()
void loadFromString(String)
void save(java.io.File)
void save(String)
void load(java.io.File)
void load(String)
void load(java.io.Reader)
```

### FileConfigurationOptions (class)

```java
FileConfiguration configuration()
boolean copyDefaults()
FileConfigurationOptions copyDefaults(boolean)
String header()
FileConfigurationOptions header(String)
FileConfigurationOptions setHeader(List<String>)
FileConfigurationOptions copyHeader(boolean)
FileConfigurationOptions parseComments(boolean)
```

### YamlConfiguration (class)

```java
YamlConfiguration()
String saveToString()
void loadFromString(String)
YamlConfiguration loadConfiguration(java.io.File)
YamlConfiguration loadConfiguration(java.io.Reader)
ConfigurationSection getParent()
```

## `org.bukkit.enchantments`

### Enchantment (class)

```java
Enchantment AQUA_AFFINITY
Enchantment BANE_OF_ARTHROPODS
Enchantment BINDING_CURSE
Enchantment BLAST_PROTECTION
Enchantment BREACH
Enchantment CHANNELING
Enchantment DENSITY
Enchantment DEPTH_STRIDER
Enchantment EFFICIENCY
Enchantment FEATHER_FALLING
Enchantment FIRE_ASPECT
Enchantment FIRE_PROTECTION
Enchantment FLAME
Enchantment FORTUNE
Enchantment FROST_WALKER
Enchantment IMPALING
Enchantment INFINITY
Enchantment KNOCKBACK
Enchantment LOOTING
Enchantment LOYALTY
Enchantment LUCK_OF_THE_SEA
Enchantment LUNGE
Enchantment LURE
Enchantment MENDING
Enchantment MULTISHOT
Enchantment PIERCING
Enchantment POWER
Enchantment PROJECTILE_PROTECTION
Enchantment PROTECTION
Enchantment PUNCH
Enchantment QUICK_CHARGE
Enchantment RESPIRATION
Enchantment RIPTIDE
Enchantment SHARPNESS
Enchantment SILK_TOUCH
Enchantment SMITE
Enchantment SOUL_SPEED
Enchantment SWEEPING_EDGE
Enchantment SWIFT_SNEAK
Enchantment THORNS
Enchantment UNBREAKING
Enchantment VANISHING_CURSE
Enchantment WIND_BURST
Enchantment DAMAGE_ALL
Enchantment DAMAGE_UNDEAD
Enchantment DAMAGE_ARTHROPODS
Enchantment DURABILITY
Enchantment DIG_SPEED
Enchantment LOOT_BONUS_BLOCKS
Enchantment LOOT_BONUS_MOBS
Enchantment PROTECTION_ENVIRONMENTAL
Enchantment PROTECTION_FIRE
Enchantment PROTECTION_FALL
Enchantment PROTECTION_EXPLOSIONS
Enchantment PROTECTION_PROJECTILE
Enchantment OXYGEN
Enchantment WATER_WORKER
Enchantment ARROW_DAMAGE
Enchantment ARROW_KNOCKBACK
Enchantment ARROW_FIRE
Enchantment ARROW_INFINITE
Enchantment LUCK
NamespacedKey getKey()
NamespacedKey key()
String getName()
int getMaxLevel()
int getStartLevel()
boolean isCursed()
boolean isTreasure()
Enchantment getByKey(NamespacedKey)
Enchantment getByName(String)
Enchantment[] values()
Collection<Enchantment> all()
String toString()
```

## `org.bukkit.entity`

### Damageable (interface)

```java
double getHealth()
void setHealth(double)
double getMaxHealth()
void damage(double)
void damage(double, Entity)
```

### Entity (interface)

```java
Location getLocation()
boolean teleport(Location)
boolean teleport(Entity)
World getWorld()
UUID getUniqueId()
int getEntityId()
EntityType getType()
boolean isDead()
boolean isValid()
void remove()
boolean isOnGround()
Vector getVelocity()
void setVelocity(Vector)
int getFireTicks()
void setFireTicks(int)
```

### EntityType (class)

```java
EntityType PLAYER
EntityType ITEM
EntityType PIG
EntityType COW
EntityType SHEEP
EntityType CHICKEN
EntityType ZOMBIE
EntityType SPIDER
EntityType CREEPER
EntityType SKELETON
EntityType TNT
EntityType FALLING_BLOCK
EntityType EXPERIENCE_ORB
EntityType ARROW
EntityType UNKNOWN
EntityType[] values()
EntityType valueOf(String)
boolean isAlive()
boolean isSpawnable()
NamespacedKey getKey()
EntityType fromName(String)
```

### HumanEntity (interface)

```java
PlayerInventory getInventory()
GameMode getGameMode()
void setGameMode(GameMode)
ItemStack getItemInHand()
void closeInventory()
```

### Item (interface)

```java
ItemStack getItemStack()
int getPickupDelay()
void setPickupDelay(int)
```

### LivingEntity (interface)

```java
Location getEyeLocation()
double getEyeHeight()
int getRemainingAir()
void setRemainingAir(int)
int getMaximumAir()
```

### Player (interface)

```java
String getDisplayName()
void setDisplayName(String)
String getPlayerListName()
void setPlayerListName(String)
Component displayName()
void displayName(Component)
void playerListName(Component)
void kickPlayer(String)
void kick(Component)
void kick()
void chat(String)
boolean performCommand(String)
boolean isSneaking()
boolean isSprinting()
boolean isFlying()
int getFoodLevel()
void setFoodLevel(int)
float getSaturation()
void setSaturation(float)
int getLevel()
void setLevel(int)
float getExp()
void setExp(float)
int getTotalExperience()
void setTotalExperience(int)
void giveExp(int)
void giveExpLevels(int)
void sendTitle(String, String)
void sendTitle(String, String, int, int, int)
void sendActionBar(String)
void sendActionBar(Component)
void setPlayerListHeaderFooter(String, String)
void sendPlayerListHeaderAndFooter(Component, Component)
void playSound(Location, String, float, float)
java.net.InetSocketAddress getAddress()
int getPing()
String getLocale()
void updateInventory()
Location getCompassTarget()
Location getBedSpawnLocation()
Location getRespawnLocation()
boolean getAllowFlight()
void setAllowFlight(boolean)
void setFlying(boolean)
boolean isConnected()
String getClientBrandName()
InventoryView openInventory(Inventory)
InventoryView getOpenInventory()
Scoreboard getScoreboard()
void setScoreboard(Scoreboard)
void hidePlayer(Plugin, Player)
void showPlayer(Plugin, Player)
void hidePlayer(Player)
void showPlayer(Player)
boolean canSee(Player)
float getFlySpeed()
void setFlySpeed(float)
float getWalkSpeed()
void setWalkSpeed(float)
<T> void spawnParticle(Particle, double, double, double, int, double, double, double, double, T)
void spawnParticle(Particle, Location, int)
void spawnParticle(Particle, double, double, double, int)
<T> void spawnParticle(Particle, Location, int, T)
void spawnParticle(Particle, Location, int, double, double, double)
void spawnParticle(Particle, Location, int, double, double, double, double)
<T> void spawnParticle(Particle, Location, int, double, double, double, T)
<T> void spawnParticle(Particle, Location, int, double, double, double, double, T)
void showBossBar(BossBar)
void hideBossBar(BossBar)
```

## `org.bukkit.event`

### Cancellable (interface)

```java
boolean isCancelled()
void setCancelled(boolean)
```

### Event (class)

```java
Event()
Event(boolean)
String getEventName()
HandlerList getHandlers()
boolean isAsynchronous()
boolean callEvent()
```

### EventException (class)

```java
EventException(Throwable)
EventException(Throwable, String)
EventException(String)
```

### EventHandler (interface)

```java
EventPriority priority()
boolean ignoreCancelled()
```

### EventPriority (class)

```java
EventPriority LOWEST
EventPriority LOW
EventPriority NORMAL
EventPriority HIGH
EventPriority HIGHEST
EventPriority MONITOR
EventPriority[] values()
EventPriority valueOf(String)
int getSlot()
```

### HandlerList (class)

```java
HandlerList()
void unregisterAll(Plugin)
void unregisterAll(Listener)
void unregister(Plugin)
void unregister(Listener)
void unregisterAll()
```

### Listener (interface)

## `org.bukkit.event.block`

### Action (class)

```java
Action LEFT_CLICK_BLOCK
Action RIGHT_CLICK_BLOCK
Action LEFT_CLICK_AIR
Action RIGHT_CLICK_AIR
Action PHYSICAL
Action[] values()
Action valueOf(String)
boolean isLeftClick()
boolean isRightClick()
```

### BlockBreakEvent (class)

```java
BlockBreakEvent(Block, Player, boolean, int)
Player getPlayer()
boolean isDropItems()
void setDropItems(boolean)
int getExpToDrop()
void setExpToDrop(int)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### BlockEvent (class)

```java
BlockEvent(Block)
Block getBlock()
```

### BlockPlaceEvent (class)

```java
BlockPlaceEvent(Block, Player, Block, ItemStack, boolean)
Block getBlockPlaced()
Player getPlayer()
Block getBlockAgainst()
ItemStack getItemInHand()
boolean canBuild()
void setBuild(boolean)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

## `org.bukkit.event.entity`

### CreatureSpawnEvent (class)

```java
CreatureSpawnEvent(Entity, CreatureSpawnEvent$SpawnReason)
CreatureSpawnEvent$SpawnReason getSpawnReason()
Location getLocation()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### EntityDamageByEntityEvent (class)

```java
EntityDamageByEntityEvent(Entity, Entity, EntityDamageEvent$DamageCause, double)
Entity getDamager()
HandlerList getHandlers()
HandlerList getHandlerList()
```

### EntityDamageEvent (class)

```java
EntityDamageEvent(Entity, EntityDamageEvent$DamageCause, double)
EntityDamageEvent$DamageCause getCause()
double getFinalDamage()
double getDamage()
void setDamage(double)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### EntityDeathEvent (class)

```java
EntityDeathEvent(Entity, List<ItemStack>, int)
LivingEntity getEntity()
List<ItemStack> getDrops()
int getDroppedExp()
void setDroppedExp(int)
HandlerList getHandlers()
HandlerList getHandlerList()
Entity getEntity()
```

### EntityEvent (class)

```java
EntityEvent(Entity)
Entity getEntity()
EntityType getEntityType()
```

### EntityExplodeEvent (class)

```java
EntityExplodeEvent(Entity, Location, List<Block>, float)
Location getLocation()
List<Block> blockList()
float getYield()
void setYield(float)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### EntityPickupItemEvent (class)

```java
EntityPickupItemEvent(LivingEntity, Item, int)
Item getItem()
int getRemaining()
LivingEntity getEntity()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
Entity getEntity()
```

### FoodLevelChangeEvent (class)

```java
FoodLevelChangeEvent(HumanEntity, int)
int getFoodLevel()
void setFoodLevel(int)
HumanEntity getEntity()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
Entity getEntity()
```

### PlayerDeathEvent (class)

```java
PlayerDeathEvent(Player, List<ItemStack>, int, String)
Player getEntity()
Player getPlayer()
String getDeathMessage()
void setDeathMessage(String)
Component deathMessage()
void deathMessage(Component)
boolean getKeepInventory()
void setKeepInventory(boolean)
boolean getKeepLevel()
void setKeepLevel(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
LivingEntity getEntity()
Entity getEntity()
```

## `org.bukkit.event.inventory`

### ClickType (class)

```java
ClickType LEFT
ClickType SHIFT_LEFT
ClickType RIGHT
ClickType SHIFT_RIGHT
ClickType WINDOW_BORDER_LEFT
ClickType WINDOW_BORDER_RIGHT
ClickType MIDDLE
ClickType NUMBER_KEY
ClickType DOUBLE_CLICK
ClickType DROP
ClickType CONTROL_DROP
ClickType CREATIVE
ClickType SWAP_OFFHAND
ClickType UNKNOWN
ClickType[] values()
ClickType valueOf(String)
boolean isKeyboardClick()
boolean isCreativeAction()
boolean isRightClick()
boolean isLeftClick()
boolean isShiftClick()
ClickType fromVanilla(int, int, int)
```

### InventoryAction (class)

```java
InventoryAction NOTHING
InventoryAction PICKUP_ALL
InventoryAction PICKUP_SOME
InventoryAction PICKUP_HALF
InventoryAction PICKUP_ONE
InventoryAction PLACE_ALL
InventoryAction PLACE_SOME
InventoryAction PLACE_ONE
InventoryAction SWAP_WITH_CURSOR
InventoryAction DROP_ALL_CURSOR
InventoryAction DROP_ONE_CURSOR
InventoryAction DROP_ALL_SLOT
InventoryAction DROP_ONE_SLOT
InventoryAction MOVE_TO_OTHER_INVENTORY
InventoryAction HOTBAR_MOVE_AND_READD
InventoryAction HOTBAR_SWAP
InventoryAction CLONE_STACK
InventoryAction COLLECT_TO_CURSOR
InventoryAction UNKNOWN
InventoryAction[] values()
InventoryAction valueOf(String)
```

### InventoryClickEvent (class)

```java
InventoryClickEvent(InventoryView, InventoryType$SlotType, int, ClickType, InventoryAction)
InventoryClickEvent(InventoryView, InventoryType$SlotType, int, ClickType, InventoryAction, int)
HumanEntity getWhoClicked()
int getRawSlot()
int getSlot()
Inventory getClickedInventory()
InventoryType$SlotType getSlotType()
ClickType getClick()
InventoryAction getAction()
int getHotbarButton()
boolean isLeftClick()
boolean isRightClick()
boolean isShiftClick()
ItemStack getCurrentItem()
void setCurrentItem(ItemStack)
ItemStack getCursor()
void setCreeItems(ItemStack, ItemStack)
Event$Result getResult()
void setResult(Event$Result)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### InventoryCloseEvent (class)

```java
InventoryCloseEvent(InventoryView)
InventoryCloseEvent(InventoryView, InventoryCloseEvent$Reason)
HumanEntity getPlayer()
InventoryCloseEvent$Reason getReason()
HandlerList getHandlers()
HandlerList getHandlerList()
```

### InventoryEvent (class)

```java
InventoryEvent(InventoryView)
Inventory getInventory()
InventoryView getView()
List<HumanEntity> getViewers()
```

### InventoryOpenEvent (class)

```java
InventoryOpenEvent(InventoryView)
HumanEntity getPlayer()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### InventoryType (class)

```java
InventoryType CHEST
InventoryType DISPENSER
InventoryType DROPPER
InventoryType FURNACE
InventoryType WORKBENCH
InventoryType CRAFTING
InventoryType ENCHANTING
InventoryType PLAYER
InventoryType CREATIVE
InventoryType ANVIL
InventoryType ENDER_CHEST
InventoryType HOPPER
InventoryType SHULKER_BOX
InventoryType BARREL
InventoryType BLAST_FURNACE
InventoryType SMOKER
InventoryType MERCHANT
InventoryType BREWING
InventoryType[] values()
InventoryType valueOf(String)
int getDefaultSize()
String getDefaultTitle()
boolean isCreatable()
InventoryType fromCree(String)
```

## `org.bukkit.event.player`

### AsyncPlayerChatEvent (class)

```java
AsyncPlayerChatEvent(boolean, Player, String, Set<Player>)
String getMessage()
void setMessage(String)
String getFormat()
void setFormat(String)
Set<Player> getRecipients()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### AsyncPlayerPreLoginEvent (class)

```java
AsyncPlayerPreLoginEvent(String, java.net.InetAddress, UUID)
String getName()
UUID getUniqueId()
java.net.InetAddress getAddress()
AsyncPlayerPreLoginEvent$Result getLoginResult()
void setLoginResult(AsyncPlayerPreLoginEvent$Result)
String getKickMessage()
void setKickMessage(String)
void kickMessage(Component)
void allow()
void disallow(AsyncPlayerPreLoginEvent$Result, String)
void disallow(AsyncPlayerPreLoginEvent$Result, Component)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerCommandPreprocessEvent (class)

```java
PlayerCommandPreprocessEvent(Player, String)
String getMessage()
void setMessage(String)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerDropItemEvent (class)

```java
PlayerDropItemEvent(Player, Item)
Item getItemDrop()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerEvent (class)

```java
PlayerEvent(Player)
PlayerEvent(Player, boolean)
Player getPlayer()
```

### PlayerGameModeChangeEvent (class)

```java
PlayerGameModeChangeEvent(Player, GameMode)
GameMode getNewGameMode()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerInteractEntityEvent (class)

```java
PlayerInteractEntityEvent(Player, Entity)
Entity getRightClicked()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerInteractEvent (class)

```java
PlayerInteractEvent(Player, Action, ItemStack, Block, BlockFace, EquipmentSlot)
boolean hasItem()
boolean hasBlock()
Material getMaterial()
Action getAction()
ItemStack getItem()
Block getClickedBlock()
BlockFace getBlockFace()
EquipmentSlot getHand()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerItemConsumeEvent (class)

```java
PlayerItemConsumeEvent(Player, ItemStack)
ItemStack getItem()
void setItem(ItemStack)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerItemHeldEvent (class)

```java
PlayerItemHeldEvent(Player, int, int)
int getPreviousSlot()
int getNewSlot()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerJoinEvent (class)

```java
PlayerJoinEvent(Player, String)
Component joinMessage()
void joinMessage(Component)
String getJoinMessage()
void setJoinMessage(String)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerKickEvent (class)

```java
PlayerKickEvent(Player, String, String)
String getReason()
void setReason(String)
String getLeaveMessage()
void setLeaveMessage(String)
Component reason()
void reason(Component)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerMoveEvent (class)

```java
PlayerMoveEvent(Player, Location, Location)
Location getFrom()
void setFrom(Location)
Location getTo()
void setTo(Location)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerQuitEvent (class)

```java
PlayerQuitEvent(Player, String)
Component quitMessage()
void quitMessage(Component)
String getQuitMessage()
void setQuitMessage(String)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerRespawnEvent (class)

```java
PlayerRespawnEvent(Player, Location)
Location getRespawnLocation()
void setRespawnLocation(Location)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerSwapHandItemsEvent (class)

```java
PlayerSwapHandItemsEvent(Player, ItemStack, ItemStack)
ItemStack getMainHandItem()
void setMainHandItem(ItemStack)
ItemStack getOffHandItem()
void setOffHandItem(ItemStack)
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerToggleFlightEvent (class)

```java
PlayerToggleFlightEvent(Player, boolean)
boolean isFlying()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerToggleSneakEvent (class)

```java
PlayerToggleSneakEvent(Player, boolean)
boolean isSneaking()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

### PlayerToggleSprintEvent (class)

```java
PlayerToggleSprintEvent(Player, boolean)
boolean isSprinting()
boolean isCancelled()
void setCancelled(boolean)
HandlerList getHandlers()
HandlerList getHandlerList()
```

## `org.bukkit.event.server`

### PluginEnableEvent (class)

```java
PluginEnableEvent(Plugin)
Plugin getPlugin()
HandlerList getHandlers()
HandlerList getHandlerList()
```

### ServerListPingEvent (class)

```java
ServerListPingEvent(String, String, int, int)
String getHostname()
String getMotd()
void setMotd(String)
Component motd()
void motd(Component)
int getNumPlayers()
int getMaxPlayers()
void setMaxPlayers(int)
HandlerList getHandlers()
HandlerList getHandlerList()
```

## `org.bukkit.inventory`

### EquipmentSlot (class)

```java
EquipmentSlot HAND
EquipmentSlot OFF_HAND
EquipmentSlot FEET
EquipmentSlot LEGS
EquipmentSlot CHEST
EquipmentSlot HEAD
EquipmentSlot BODY
EquipmentSlot SADDLE
EquipmentSlot[] values()
EquipmentSlot valueOf(String)
```

### Inventory (interface)

```java
int getSize()
ItemStack getItem(int)
void setItem(int, ItemStack)
HashMap<Integer, ItemStack> addItem(..)
HashMap<Integer, ItemStack> removeItem(..)
ItemStack[] getContents()
void setContents(ItemStack[])
boolean contains(Material)
boolean contains(Material, int)
boolean containsAtLeast(ItemStack, int)
int firstEmpty()
boolean isEmpty()
void clear()
void clear(int)
void remove(Material)
void remove(ItemStack)
InventoryHolder getHolder()
List<HumanEntity> getViewers()
InventoryType getType()
int first(Material)
int first(ItemStack)
boolean contains(ItemStack)
HashMap<Integer, ? extends ItemStack> all(Material)
int getMaxStackSize()
ItemStack[] getStorageContents()
void setStorageContents(ItemStack[])
Location getLocation()
void close()
```

### InventoryHolder (interface)

```java
Inventory getInventory()
```

### InventoryView (class)

```java
InventoryView(Inventory, Inventory, HumanEntity, String, InventoryType)
Inventory getTopInventory()
Inventory getBottomInventory()
HumanEntity getPlayer()
InventoryType getType()
String getTitle()
String getOriginalTitle()
Component title()
int countSlots()
Inventory getInventory(int)
int convertSlot(int)
ItemStack getItem(int)
void setItem(int, ItemStack)
void close()
```

### ItemFactory (class)

```java
ItemFactory INSTANCE
ItemMeta getItemMeta(Material)
boolean isApplicable(ItemMeta, Material)
boolean equals(ItemMeta, ItemMeta)
ItemMeta asMetaFor(ItemMeta, Material)
```

### ItemFlag (class)

```java
ItemFlag HIDE_ENCHANTS
ItemFlag HIDE_ATTRIBUTES
ItemFlag HIDE_UNBREAKABLE
ItemFlag HIDE_DESTROYS
ItemFlag HIDE_PLACED_ON
ItemFlag HIDE_ADDITIONAL_TOOLTIP
ItemFlag HIDE_DYE
ItemFlag HIDE_ARMOR_TRIM
ItemFlag HIDE_STORED_ENCHANTS
ItemFlag[] values()
ItemFlag valueOf(String)
```

### ItemStack (class)

```java
ItemStack(Material)
ItemStack(Material, int)
ItemStack(ItemStack)
ItemStack of(Material)
ItemStack of(Material, int)
ItemStack empty()
Material getType()
void setType(Material)
int getAmount()
void setAmount(int)
int getMaxStackSize()
boolean isEmpty()
boolean hasItemMeta()
ItemMeta getItemMeta()
boolean setItemMeta(ItemMeta)
boolean isSimilar(ItemStack)
void addEnchantment(Enchantment, int)
void addUnsafeEnchantment(Enchantment, int)
void addEnchantments(Map<Enchantment, Integer>)
void addUnsafeEnchantments(Map<Enchantment, Integer>)
int removeEnchantment(Enchantment)
boolean containsEnchantment(Enchantment)
int getEnchantmentLevel(Enchantment)
Map<Enchantment, Integer> getEnchantments()
ItemStack add(int)
ItemStack subtract(int)
ItemStack asQuantity(int)
ItemStack asOne()
Component displayName()
boolean editMeta(function.Consumer<? super ItemMeta>)
void lore(List<? extends Component>)
List<Component> lore()
ItemStack clone()
boolean equals(Object)
int hashCode()
String toString()
String toCreeItem()
ItemStack fromCree(String, int)
Object clone()
```

### PlayerInventory (interface)

```java
ItemStack getItemInMainHand()
void setItemInMainHand(ItemStack)
ItemStack getItemInOffHand()
void setItemInOffHand(ItemStack)
int getHeldItemSlot()
void setHeldItemSlot(int)
ItemStack getHelmet()
ItemStack getChestplate()
ItemStack getLeggings()
ItemStack getBoots()
void setHelmet(ItemStack)
void setChestplate(ItemStack)
void setLeggings(ItemStack)
void setBoots(ItemStack)
ItemStack[] getArmorContents()
```

## `org.bukkit.inventory.meta`

### ItemMeta (interface)

```java
boolean hasDisplayName()
String getDisplayName()
void setDisplayName(String)
Component displayName()
void displayName(Component)
boolean hasItemName()
void itemName(Component)
boolean hasLore()
List<String> getLore()
void setLore(List<String>)
List<Component> lore()
void lore(List<? extends Component>)
boolean hasEnchants()
boolean hasEnchant(Enchantment)
int getEnchantLevel(Enchantment)
Map<Enchantment, Integer> getEnchants()
boolean addEnchant(Enchantment, int, boolean)
boolean removeEnchant(Enchantment)
void removeEnchantments()
boolean hasConflictingEnchant(Enchantment)
boolean isUnbreakable()
void setUnbreakable(boolean)
boolean hasCustomModelData()
int getCustomModelData()
void setCustomModelData(Integer)
boolean hasEnchantmentGlintOverride()
Boolean getEnchantmentGlintOverride()
void setEnchantmentGlintOverride(Boolean)
void addItemFlags(..)
void removeItemFlags(..)
Set<ItemFlag> getItemFlags()
boolean hasItemFlag(ItemFlag)
boolean isEmpty()
ItemMeta clone()
```

## `org.bukkit.permissions`

### Permissible (interface)

```java
boolean hasPermission(String)
boolean isPermissionSet(String)
```

### ServerOperator (interface)

```java
boolean isOp()
void setOp(boolean)
```

## `org.bukkit.plugin`

### EventExecutor (interface)

```java
void execute(Listener, Event)
```

### Plugin (interface)

```java
java.io.File getDataFolder()
PluginDescriptionFile getDescription()
FileConfiguration getConfig()
java.io.InputStream getResource(String)
void saveConfig()
void saveDefaultConfig()
void saveResource(String, boolean)
void reloadConfig()
Server getServer()
boolean isEnabled()
void onDisable()
void onLoad()
void onEnable()
boolean isNaggable()
void setNaggable(boolean)
logging.Logger getLogger()
String getName()
```

### PluginBase (class)

```java
PluginBase()
int hashCode()
boolean equals(Object)
String getName()
```

### PluginDescriptionFile (class)

```java
PluginDescriptionFile(Map<String, Object>)
PluginDescriptionFile(String, String, String)
String getName()
String getVersion()
String getFullName()
String getMain()
String getDescription()
String getWebsite()
String getPrefix()
String getAPIVersion()
List<String> getAuthors()
List<String> getDepend()
List<String> getSoftDepend()
List<String> getLoadBefore()
Map<String, Map<String, Object>> getCommands()
Map<String, Object> getPermissionsRaw()
Map<String, Object> getRaw()
```

### PluginManager (interface)

```java
Plugin getPlugin(String)
Plugin[] getPlugins()
boolean isPluginEnabled(String)
boolean isPluginEnabled(Plugin)
void callEvent(Event)
void registerEvents(Listener, Plugin)
void registerEvent(Class<? extends Event>, Listener, EventPriority, EventExecutor, Plugin)
void registerEvent(Class<? extends Event>, Listener, EventPriority, EventExecutor, Plugin, boolean)
void disablePlugin(Plugin)
void enablePlugin(Plugin)
```

## `org.bukkit.plugin.java`

### JavaPlugin (class)

```java
ThreadLocal<Object[]> INIT
JavaPlugin()
void init(PluginDescriptionFile, java.io.File, java.io.File, ClassLoader)
java.io.File getDataFolder()
PluginDescriptionFile getDescription()
PluginDescriptionFile getPluginMeta()
Server getServer()
boolean isEnabled()
void setEnabled(boolean)
logging.Logger getLogger()
org.slf4j.Logger getSLF4JLogger()
ComponentLogger getComponentLogger()
FileConfiguration getConfig()
void reloadConfig()
void saveConfig()
void saveDefaultConfig()
void saveResource(String, boolean)
java.io.InputStream getResource(String)
PluginCommand getCommand(String)
void registerCommand(String, BasicCommand)
void registerCommand(String, String, BasicCommand)
void registerCommand(String, Collection<String>, BasicCommand)
void registerCommand(String, String, Collection<String>, BasicCommand)
boolean onCommand(CommandSender, Command, String, String[])
List<String> onTabComplete(CommandSender, Command, String, String[])
void onLoad()
void onDisable()
void onEnable()
boolean isNaggable()
void setNaggable(boolean)
String toString()
<T extends JavaPlugin> T getPlugin(Class<T>)
JavaPlugin getProvidingPlugin(Class<?>)
```

## `org.bukkit.scheduler`

### BukkitRunnable (class)

```java
BukkitRunnable()
synchronized boolean isCancelled()
synchronized void cancel()
synchronized BukkitTask runTask(Plugin)
synchronized BukkitTask runTaskAsynchronously(Plugin)
synchronized BukkitTask runTaskLater(Plugin, long)
synchronized BukkitTask runTaskLaterAsynchronously(Plugin, long)
synchronized BukkitTask runTaskTimer(Plugin, long, long)
synchronized BukkitTask runTaskTimerAsynchronously(Plugin, long, long)
synchronized int getTaskId()
```

### BukkitScheduler (interface)

```java
BukkitTask runTask(Plugin, Runnable)
void runTask(Plugin, function.Consumer<? super BukkitTask>)
BukkitTask runTaskLater(Plugin, Runnable, long)
void runTaskLater(Plugin, function.Consumer<? super BukkitTask>, long)
BukkitTask runTaskTimer(Plugin, Runnable, long, long)
void runTaskTimer(Plugin, function.Consumer<? super BukkitTask>, long, long)
BukkitTask runTaskAsynchronously(Plugin, Runnable)
void runTaskAsynchronously(Plugin, function.Consumer<? super BukkitTask>)
BukkitTask runTaskLaterAsynchronously(Plugin, Runnable, long)
BukkitTask runTaskTimerAsynchronously(Plugin, Runnable, long, long)
int scheduleSyncDelayedTask(Plugin, Runnable, long)
int scheduleSyncDelayedTask(Plugin, Runnable)
int scheduleSyncRepeatingTask(Plugin, Runnable, long, long)
<T> concurrent.Future<T> callSyncMethod(Plugin, concurrent.Callable<T>)
void cancelTask(int)
void cancelTasks(Plugin)
boolean isCurrentlyRunning(int)
boolean isQueued(int)
```

### BukkitTask (interface)

```java
int getTaskId()
Plugin getOwner()
boolean isSync()
boolean isCancelled()
void cancel()
```

## `org.bukkit.scoreboard`

### Criteria (class)

```java
Criteria DUMMY
Criteria HEALTH
Criteria DEATH_COUNT
Criteria PLAYER_KILL_COUNT
Criteria TOTAL_KILL_COUNT
Criteria LEVEL
Criteria FOOD
Criteria XP
Criteria create(String)
String getName()
boolean isReadOnly()
RenderType getDefaultRenderType()
```

### DisplaySlot (class)

```java
DisplaySlot PLAYER_LIST
DisplaySlot SIDEBAR
DisplaySlot BELOW_NAME
DisplaySlot[] values()
DisplaySlot valueOf(String)
int getId()
```

### Objective (interface)

```java
String getName()
String getDisplayName()
void setDisplayName(String)
Component displayName()
void displayName(Component)
String getCriteria()
Criteria getTrackedCriteria()
boolean isModifiable()
Scoreboard getScoreboard()
void unregister()
void setDisplaySlot(DisplaySlot)
DisplaySlot getDisplaySlot()
void setRenderType(RenderType)
RenderType getRenderType()
Score getScore(String)
Score getScore(OfflinePlayer)
```

### RenderType (class)

```java
RenderType INTEGER
RenderType HEARTS
RenderType[] values()
RenderType valueOf(String)
```

### Score (interface)

```java
String getEntry()
Objective getObjective()
int getScore()
void setScore(int)
boolean isScoreSet()
void resetScore()
Scoreboard getScoreboard()
void customName(Component)
Component customName()
```

### Scoreboard (interface)

```java
Objective registerNewObjective(String, String)
Objective registerNewObjective(String, String, String)
Objective registerNewObjective(String, String, String, RenderType)
Objective registerNewObjective(String, Criteria, String)
Objective registerNewObjective(String, Criteria, String, RenderType)
Objective registerNewObjective(String, Criteria, Component)
Objective registerNewObjective(String, Criteria, Component, RenderType)
Objective getObjective(String)
Objective getObjective(DisplaySlot)
Set<Objective> getObjectives()
Set<Objective> getObjectivesByCriteria(String)
void clearSlot(DisplaySlot)
Team registerNewTeam(String)
Team getTeam(String)
Set<Team> getTeams()
Team getEntryTeam(String)
Team getPlayerTeam(OfflinePlayer)
Set<Score> getScores(String)
void resetScores(String)
void resetScores(OfflinePlayer)
Set<String> getEntries()
```

### ScoreboardManager (interface)

```java
Scoreboard getMainScoreboard()
Scoreboard getNewScoreboard()
```

### Team (interface)

```java
String getName()
String getDisplayName()
void setDisplayName(String)
String getPrefix()
void setPrefix(String)
String getSuffix()
void setSuffix(String)
Component prefix()
void prefix(Component)
Component suffix()
void suffix(Component)
Component displayName()
void displayName(Component)
ChatColor getColor()
void setColor(ChatColor)
boolean allowFriendlyFire()
void setAllowFriendlyFire(boolean)
boolean canSeeFriendlyInvisibles()
void setCanSeeFriendlyInvisibles(boolean)
Team$OptionStatus getOption(Team$Option)
void setOption(Team$Option, Team$OptionStatus)
Set<String> getEntries()
int getSize()
Scoreboard getScoreboard()
void addEntry(String)
void addEntries(String...)
boolean removeEntry(String)
boolean hasEntry(String)
void addPlayer(OfflinePlayer)
boolean removePlayer(OfflinePlayer)
boolean hasPlayer(OfflinePlayer)
void unregister()
```

## `org.bukkit.util`

### Vector (class)

```java
Vector()
Vector(int, int, int)
Vector(float, float, float)
Vector(double, double, double)
double getX()
double getY()
double getZ()
Vector setX(double)
Vector setY(double)
Vector setZ(double)
Vector add(Vector)
Vector subtract(Vector)
Vector multiply(double)
Vector multiply(int)
Vector multiply(Vector)
double length()
double lengthSquared()
Vector normalize()
double distance(Vector)
double distanceSquared(Vector)
double dot(Vector)
Vector crossProduct(Vector)
Vector zero()
int getBlockX()
int getBlockY()
int getBlockZ()
Location toLocation(World)
Location toLocation(World, float, float)
Vector clone()
boolean equals(Object)
int hashCode()
String toString()
Object clone()
```

## `org.slf4j`

### Logger (interface)

```java
String getName()
boolean isDebugEnabled()
boolean isInfoEnabled()
void trace(String)
void trace(String, Object...)
void debug(String)
void debug(String, Object)
void debug(String, Object...)
void debug(String, Throwable)
void info(String)
void info(String, Object)
void info(String, Object, Object)
void info(String, Object...)
void info(String, Throwable)
void warn(String)
void warn(String, Object)
void warn(String, Object, Object)
void warn(String, Object...)
void warn(String, Throwable)
void error(String)
void error(String, Object)
void error(String, Object, Object)
void error(String, Object...)
void error(String, Throwable)
```

