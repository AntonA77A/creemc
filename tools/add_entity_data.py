"""Ergänzt data/game.json um Daten aller Entities (Größe, Leben, Schaden, Tempo, Kategorie, Sounds) und die
Loot-Tabellen der Mobs.

Quellen: Pumpkin-MC/Pumpkin (entities.json der neuesten Version), misode/mcmeta (loot_table).
Benutzung (im Ordner creemc-cpp):  python tools/add_entity_data.py
"""
import json
import os
import urllib.request

PUMPKIN = "https://raw.githubusercontent.com/Pumpkin-MC/Pumpkin/9bab5ea4/assets/entities.json"
MCMETA = "https://raw.githubusercontent.com/misode/mcmeta/26.3-summary/data/loot_table/data.min.json"
GAME = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "game.json")


def fetch(url):
    with urllib.request.urlopen(url) as r:
        return json.loads(r.read())


def main():
    with open(GAME, encoding="utf-8") as f:
        game = json.load(f)
    info = {}
    for name, e in fetch(PUMPKIN).items():
        attrs = {}
        for a in e.get("attributes", []):
            attrs.update(a)
        w, h = e.get("dimension", [0.6, 1.8])
        info["minecraft:" + name] = {
            "w": w, "h": h, "eye": e.get("eye_height", h * 0.85),
            "health": e.get("max_health", attrs.get("max_health", 0)) or 0,
            "xp": e.get("experience_reward", 0),
            "cat": e.get("category", "MISC"),
            "mob": bool(e.get("mob")),
            "fire_immune": bool(e.get("fire_immune")),
            "speed": attrs.get("movement_speed", 0.25),
            "fly": attrs.get("flying_speed", 0),
            "damage": attrs.get("attack_damage", 0),
            "armor": attrs.get("armor", 0),
            "follow": attrs.get("follow_range", 16),
            "summonable": bool(e.get("summonable", True)),
            "spawn": (e.get("spawn_restriction") or {}).get("location", "NO_RESTRICTIONS"),
            "hurt": e.get("hurt_sound"), "death": e.get("death_sound"),
        }
    loot = {}
    for k, v in fetch(MCMETA).items():
        if k.startswith("entities/"):
            loot["minecraft:" + k.removeprefix("entities/")] = v   # z.B. "minecraft:sheep/white"
    game["entity_info"] = info
    game["entity_loot"] = loot
    with open(GAME, "w", encoding="utf-8", newline="\n") as f:
        json.dump(game, f, ensure_ascii=False, separators=(",", ":"))
    mobs = [n for n, i in info.items() if i["mob"]]
    print(f"{len(info)} Entities ({len(mobs)} Mobs), {len(loot)} Loot-Tabellen")


if __name__ == "__main__":
    main()
