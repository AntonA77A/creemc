package beispiel;

import creemc.api.CreeMC;
import creemc.api.CreeMCPlugin;
import java.util.List;
import org.bukkit.entity.Player;
import org.bukkit.event.block.BlockBreakEvent;
import org.bukkit.event.player.PlayerJoinEvent;

/** Beispiel für ein eigenes CreeMC-Plugin (creemc.yml + CreeMCPlugin). */
public class BeispielPlugin extends CreeMCPlugin {

    @Override
    public void onEnable() {
        saveDefaultConfig();
        String gruss = getConfig().getString("gruss", "Willkommen auf dem Server!");

        // Events mit Lambdas
        on(PlayerJoinEvent.class, e -> {
            CreeMC.title(e.getPlayer(), "&6Hallo " + e.getPlayer().getName(), "&7" + gruss);
            msg(e.getPlayer(), "&aBeispiel-Plugin: probier /boom, /heilen, /zauberschwert, /regel");
        });
        on(BlockBreakEvent.class, e -> {
            if (e.getBlock().getType().name().equals("DIAMOND_ORE")) CreeMC.giveXp(e.getPlayer(), 10);
        });

        // Befehle ohne yml-Eintrag
        command("boom", "Explosion vor dir (nur OP)", true, (sender, args) -> {
            if (!(sender instanceof Player p)) throw new IllegalArgumentException("Nur für Spieler");
            double power = args.length > 0 ? Double.parseDouble(args[0]) : 3;
            var l = p.getLocation();
            CreeMC.explode(l.getX(), l.getY(), l.getZ() + 4, power, true);
        });
        command("heilen", "Heilt dich", (sender, args) -> {
            if (sender instanceof Player p) {
                CreeMC.heal(p);
                msg(p, "&aGeheilt!");
            }
        });
        command("zauberschwert", "Gibt ein verzaubertes Schwert", true, (sender, args) -> {
            if (sender instanceof Player p) {
                CreeMC.give(p, CreeMC.item("diamond_sword").enchant("sharpness", 5).enchant("fire_aspect", 2)
                        .name("&6&lCreeper-Klinge"), 1);
                msg(p, "&dViel Spaß damit!");
            }
        });
        command("regel", "Zeigt oder setzt eine Spielregel", true, (sender, args) -> {
            if (args.length == 0) throw new IllegalArgumentException("Benutzung: /regel <name> [wert]");
            if (args.length == 1) msg(sender, "&e" + args[0] + " = &f" + CreeMC.gameRule(args[0]));
            else if (CreeMC.setGameRule(args[0], args[1])) msg(sender, "&a" + args[0] + " ist jetzt " + args[1]);
            else msg(sender, "&cUnbekannte Regel oder falscher Wert");
        }, (sender, args) -> args.length <= 1 ? List.copyOf(CreeMC.gameRules().keySet()) : List.of("true", "false"));
        command("mob", "Erzeugt einen Mob", true, (sender, args) -> {
            if (!(sender instanceof Player p) || args.length == 0) throw new IllegalArgumentException("Benutzung: /mob <typ>");
            var l = p.getLocation();
            int id = CreeMC.spawnMob(args[0], l.getX(), l.getY(), l.getZ());
            msg(p, id >= 0 ? "&aMob #" + id + " erzeugt" : "&cMobs: " + CreeMC.mobTypes());
        }, (sender, args) -> CreeMC.mobTypes());

        // ---- API 2: GUI, Seitenleiste, Bossbar, Partikel, Fliegen, Lore ----
        on(PlayerJoinEvent.class, e -> {
            Player p = e.getPlayer();
            CreeMC.sidebar(p, "&6&lCreeMC").lines("&7Willkommen,", "&f" + p.getName(), "", "&eBefehl: &f/menu");
            CreeMC.bossBar(p, "&bBeispiel-Bossbar", 0.75, "blue");
        });
        command("menu", "Öffnet ein Menü", (sender, args) -> {
            if (!(sender instanceof Player p)) return;
            gui("&6&lMenü", 3)
                    .item(11, CreeMC.item("golden_apple").name("&aHeilen"), (pl, click) -> {
                        CreeMC.heal(pl);
                        msg(pl, "&aGeheilt über das Menü!");
                        pl.closeInventory();
                    })
                    .item(13, CreeMC.item("feather").name("&bFliegen an/aus"), (pl, click) -> {
                        pl.setAllowFlight(!pl.getAllowFlight());
                        msg(pl, "&bFliegen: " + (pl.getAllowFlight() ? "an" : "aus"));
                    })
                    .item(15, CreeMC.item("blaze_powder").name("&cPartikel"), (pl, click) -> {
                        var l = pl.getLocation();
                        pl.getWorld().spawnParticle(org.bukkit.Particle.HEART, l.clone().add(0, 2, 0), 10, 0.5, 0.3, 0.5);
                        CreeMC.dust(l.getX(), l.getY() + 1, l.getZ(), 0xFF00FF, 1.5f, 20);
                    })
                    .fill(CreeMC.item("gray_stained_glass_pane").name(" "))
                    .open(p);
        });
        command("lore", "Gibt ein Item mit Beschreibung", (sender, args) -> {
            if (!(sender instanceof Player p)) return;
            var stack = new org.bukkit.inventory.ItemStack(org.bukkit.Material.DIAMOND_PICKAXE);
            stack.editMeta(m -> {
                m.setDisplayName("§bSuper-Spitzhacke");
                m.setLore(List.of("§7Gebaut mit der", "§7Bukkit-ItemMeta-API"));
                m.addEnchant(org.bukkit.enchantments.Enchantment.EFFICIENCY, 5, false);
                m.setUnbreakable(true);
            });
            p.getInventory().addItem(stack);
        });

        // Wiederkehrende Aufgabe
        every(20 * 60 * 10, () -> broadcast("&7[Beispiel] Denk ans Trinken!"));
        getLogger().info("Beispiel-Plugin aktiv (CreeMC-API " + CreeMC.API_VERSION + ")");
    }
}
