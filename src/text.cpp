#include "text.hpp"

#include <cctype>
#include <map>

#include "nbt.hpp"

namespace text {

const char* const SECTION = "\xC2\xA7";

namespace {
const std::map<char, const char*> COLORS = {
    {'0', "black"}, {'1', "dark_blue"}, {'2', "dark_green"}, {'3', "dark_aqua"}, {'4', "dark_red"},
    {'5', "dark_purple"}, {'6', "gold"}, {'7', "gray"}, {'8', "dark_gray"}, {'9', "blue"}, {'a', "green"},
    {'b', "aqua"}, {'c', "red"}, {'d', "light_purple"}, {'e', "yellow"}, {'f', "white"}};
const std::map<char, const char*> FORMATS = {
    {'k', "obfuscated"}, {'l', "bold"}, {'m', "strikethrough"}, {'n', "underlined"}, {'o', "italic"}};

bool isSection(const std::string& s, size_t i) { return i + 1 < s.size() && (unsigned char)s[i] == 0xC2 && (unsigned char)s[i + 1] == 0xA7; }
}  // namespace

std::string colorize(const std::string& s, char ch) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        char n = i + 1 < s.size() ? char(std::tolower((unsigned char)s[i + 1])) : 0;
        if (s[i] == ch && n && std::string("0123456789abcdefklmnor").find(n) != std::string::npos) {
            out += SECTION;
            out += n;
            ++i;
        } else {
            out += s[i];
        }
    }
    return out;
}

std::string stripColors(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (isSection(s, i)) {
            i += 2;   // § (2 Bytes) + Code
            continue;
        }
        out += s[i];
    }
    return out;
}

json::Value component(const std::string& s) {
    json::Object root;
    if (s.find(SECTION) == std::string::npos) {
        root["text"] = s;
        return json::Value(std::move(root));
    }
    json::Array parts;
    json::Object style;
    std::string buf;
    auto flush = [&]() {
        if (buf.empty()) return;
        json::Object part;
        part["text"] = buf;
        for (const auto& [k, v] : style) part[k] = v;
        parts.emplace_back(std::move(part));
        buf.clear();
    };
    for (size_t i = 0; i < s.size(); ++i) {
        if (isSection(s, i) && i + 2 < s.size()) {
            char code = char(std::tolower((unsigned char)s[i + 2]));
            i += 2;
            if (COLORS.count(code)) {
                flush();
                style = json::Object();
                style["color"] = COLORS.at(code);
            } else if (FORMATS.count(code)) {
                flush();
                style[FORMATS.at(code)] = true;
            } else if (code == 'r') {
                flush();
                style = json::Object();
            }
            continue;
        }
        buf += s[i];
    }
    flush();
    root["text"] = "";
    if (!parts.empty()) root["extra"] = json::Value(std::move(parts));
    return json::Value(std::move(root));
}

namespace {
nbt::Tag toTag(const json::Value& v) {
    switch (v.type()) {
        case json::Value::Type::Bool: return nbt::Tag::boolean(v.asBool());
        case json::Value::Type::Number: return nbt::Tag::intv(int32_t(v.asInt()));
        case json::Value::Type::String: return nbt::Tag::string(v.asString());
        case json::Value::Type::Array: {
            nbt::Tag l = nbt::Tag::list();
            for (const auto& e : v.asArray()) l.add(toTag(e));
            return l;
        }
        case json::Value::Type::Object: {
            nbt::Tag c = nbt::Tag::compound();
            for (const auto& [k, e] : v.asObject()) c.put(k, toTag(e));
            return c;
        }
        default: return nbt::Tag::string("");
    }
}
}  // namespace

Bytes toNbt(const json::Value& c) { return toTag(c).encode(); }
Bytes toNbt(const std::string& s) { return toNbt(component(s)); }
std::string toJson(const std::string& s) { return json::write(component(s)); }

}  // namespace text
