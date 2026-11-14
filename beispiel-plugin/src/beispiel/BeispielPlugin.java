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

        // Wiederkehrende Aufgabe
        every(20 * 60 * 10, () -> broadcast("&7[Beispiel] Denk ans Trinken!"));
        getLogger().info("Beispiel-Plugin aktiv (CreeMC-API " + CreeMC.API_VERSION + ")");
    }
}
