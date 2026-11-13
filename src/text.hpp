// Textkomponenten: "§aHallo" -> Minecraft-Text (als NBT oder JSON).
#pragma once
#include <string>

#include "json.hpp"
#include "util.hpp"

namespace text {

std::string colorize(const std::string& s, char ch = '&');   // "&a" -> "§a"
std::string stripColors(const std::string& s);
json::Value component(const std::string& s);
Bytes toNbt(const std::string& s);
Bytes toNbt(const json::Value& component);
std::string toJson(const std::string& s);

extern const char* const SECTION;   // "§" in UTF-8

}  // namespace text
