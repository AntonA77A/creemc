// Realistische Weltgenerierung (Ansatz wie Minecraft ab 1.18): Klimawerte aus Rauschen bestimmen Gelände und Biome,
// dazu Flüsse, Ozeane, Strände, Berge mit Gipfeln, Höhlen (Käse- und Spaghetti-Höhlen), Erze nach Vanilla-Höhen,
// Tiefenschiefer, Grundgestein, Biom-Oberflächen, Bäume und Pflanzen je Biom.
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "world.hpp"

namespace worldgen {

// Alle Biome, die der Generator benutzt (Index = Biom-Nummer im Chunk)
const std::vector<std::string>& biomeNames();
int biomeIndex(const std::string& name);   // -1 = unbekannt

std::unique_ptr<Generator> createRealistic(int64_t seed);
// Für den Spawnpunkt: Geländehöhe und ob dort Wasser ist (ohne den Chunk zu erzeugen)
bool isLand(Generator& g, int x, int z, int& height);

}  // namespace worldgen
