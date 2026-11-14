"""Ergänzt die Versionsdaten (data/versions/<protokoll>.json) um weitere Pakete, Item-Komponenten und
die Partikel-Liste - ohne den Rest neu zu erzeugen.

Quellen wie beim ursprünglichen Datenwerkzeug:
  - PrismarineJS/minecraft-data  (bis 26.1)
  - Pumpkin-MC/Pumpkin           (26.2 / 26.3)
  - misode/mcmeta                (Registries, hier: particle_type)

Benutzung (im Ordner creemc-cpp):  python tools/add_packets.py
"""
import json
import os
import urllib.request

MCDATA = "https://raw.githubusercontent.com/PrismarineJS/minecraft-data/master/data/"
MCMETA = "https://raw.githubusercontent.com/misode/mcmeta/{v}-summary/"
PUMPKIN = "https://raw.githubusercontent.com/Pumpkin-MC/Pumpkin/{sha}/assets/"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "versions")

VERSIONS = {
    767: ("1.21.1", "1.21.1", None), 768: ("1.21.3", "1.21.3", None), 769: ("1.21.4", "1.21.4", None),
    770: ("1.21.5", "1.21.5", None), 771: ("1.21.6", "1.21.6", None), 772: ("1.21.8", "1.21.8", None),
    773: ("1.21.10", "1.21.10", None), 774: ("1.21.11", "1.21.11", None), 775: ("26.1", "26.1", None),
    776: (None, "26.2", "141b49f9"), 777: (None, "26.3", "9bab5ea4"),
}

# Logischer Name -> (Richtung, minecraft-data-Name, Vanilla-Name); alle im Play-Zustand
PACKETS = {
    "boss_bar": ("c", "boss_bar", "boss_event"),
    "scoreboard_objective": ("c", "scoreboard_objective", "set_objective"),
    "display_objective": ("c", "scoreboard_display_objective", "set_display_objective"),
    "update_score": ("c", "scoreboard_score", "set_score"),
    "reset_score": ("c", "reset_score", "reset_score"),
    "teams": ("c", "teams", "set_player_team"),
    "particle": ("c", "world_particles", "level_particles"),
    "update_attributes": ("c", "entity_update_attributes", "update_attributes"),
}
COMPONENTS = ["lore", "unbreakable", "custom_model_data", "enchantment_glint_override", "max_stack_size", "tooltip_display"]

_cache = {}


def fetch(url):
    if url not in _cache:
        with urllib.request.urlopen(url) as r:
            _cache[url] = json.loads(r.read())
    return _cache[url]


def main():
    for proto, (md_ver, meta_ver, sha) in VERSIONS.items():
        path = os.path.join(ROOT, f"{proto}.json")
        with open(path, encoding="utf-8") as f:
            d = json.load(f)
        added = []
        if md_ver:
            paths = fetch(MCDATA + "dataPaths.json")["pc"][md_ver]
            pj = fetch(MCDATA + paths["protocol"] + "/protocol.json")
            m = pj["play"]["toClient"]["types"]["packet"][1][0]["type"][1]["mappings"]
            ids = {v: int(k, 16) for k, v in m.items()}
            for logical, (_, md_name, _) in PACKETS.items():
                if md_name in ids:
                    d["packets"][logical] = ids[md_name]
                    added.append(logical)
            comp_ids = {n: int(i) for i, n in pj["types"]["SlotComponentType"][1]["mappings"].items()}
        else:
            base = PUMPKIN.format(sha=sha)
            pk = fetch(base + "packets.json")
            table = pk["clientbound"]["play"]
            if isinstance(table, list):
                table = {n: i for i, n in enumerate(table)}
            for logical, (_, _, van) in PACKETS.items():
                if van in table:
                    d["packets"][logical] = table[van]
                    added.append(logical)
            comp_ids = {k.removeprefix("minecraft:"): v for k, v in fetch(base + "data_component.json").items()}
        for c in COMPONENTS:
            if c in comp_ids:
                d["components"][c] = comp_ids[c]
        reg = fetch(MCMETA.format(v=meta_ver) + "registries/data.min.json")
        d["particles"] = [f"minecraft:{p}" for p in reg.get("particle_type", [])]
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            json.dump(d, f, ensure_ascii=False, separators=(",", ":"))
        missing = [p for p in PACKETS if p not in added]
        print(f"{proto}: +{len(added)} Pakete (fehlen: {', '.join(missing) or '-'}), "
              f"{len(d['particles'])} Partikel, Komponenten: {', '.join(c for c in COMPONENTS if c in d['components'])}")


if __name__ == "__main__":
    main()
