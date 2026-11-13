// Redstone nach Vanilla-Regeln: Staub, Fackeln, Hebel, Knöpfe, Druckplatten, Redstone-Block, Verstärker,
// Komparator, Beobachter, (klebrige) Kolben, Lampe, Türen, TNT, Spender/Werfer, Trichter, Notenblock.
// Kolben/Spender/Werfer prüfen auch den Block über sich (Quasi-Connectivity) - BUD-Schalter gehen.
#pragma once

class Server;

namespace redstone {

void init(Server& server);   // Block-Listener anmelden
void tick(Server& server);   // Komparatoren an Behältern regelmäßig prüfen

}  // namespace redstone
