// Server-Konfiguration (config.toml, gleiche Einträge wie die Python-Version).
#pragma once
#include <string>

struct Config {
    std::string host = "0.0.0.0";
    int port = 25565;
    std::string motd = "&a&lCreeMC";
    int maxPlayers = 20;
    int viewDistance = 8;
    int gamemode = 0;
    int difficulty = 1;
    std::string worldName = "world";
    std::string generator = "normal";
    long long seed = 12345;
    bool daylightCycle = true;
    std::string chatFormat = "&7{name}&8: &f{message}";
    std::string joinMessage = "&e{name} hat den Server betreten";
    std::string quitMessage = "&e{name} hat den Server verlassen";
    int compressionThreshold = -1;   // C++-Version: ohne Kompression (wird ignoriert)
    bool onlineMode = true;

    static Config load(const std::string& path);
};
