#include "json.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace json {

static const Value kNull;
static const std::string kEmpty;
static const Array kEmptyArray;
static const Object kEmptyObject;

// ---- Object -----------------------------------------------------------------

void Object::reindex() const {
    for (; indexed_ < items.size(); ++indexed_) index_[items[indexed_].first] = indexed_;
}

const Value* Object::find(const std::string& key) const {
    if (indexed_ > items.size()) {   // Liste wurde verkleinert
        index_.clear();
        indexed_ = 0;
    }
    reindex();
    auto it = index_.find(key);
    return it == index_.end() ? nullptr : &items[it->second].second;
}

Value* Object::find(const std::string& key) {
    return const_cast<Value*>(static_cast<const Object*>(this)->find(key));
}

Value& Object::operator[](const std::string& key) {
    if (Value* v = find(key)) return *v;
    items.emplace_back(key, Value());
    return items.back().second;
}

// ---- Value ------------------------------------------------------------------

const std::string& Value::asString() const { return type_ == Type::String ? *s_ : kEmpty; }
const Array& Value::asArray() const { return type_ == Type::Array ? *a_ : kEmptyArray; }
Array& Value::asArray() {
    if (type_ != Type::Array) {
        type_ = Type::Array;
        a_ = std::make_shared<Array>();
    }
    return *a_;
}
const Object& Value::asObject() const { return type_ == Type::Object ? *o_ : kEmptyObject; }
Object& Value::asObject() {
    if (type_ != Type::Object) {
        type_ = Type::Object;
        o_ = std::make_shared<Object>();
    }
    return *o_;
}

const Value& Value::operator[](const std::string& key) const {
    if (type_ != Type::Object) return kNull;
    const Value* v = o_->find(key);
    return v ? *v : kNull;
}

const Value& Value::operator[](size_t i) const {
    if (type_ != Type::Array || i >= a_->size()) return kNull;
    return (*a_)[i];
}

size_t Value::size() const {
    if (type_ == Type::Array) return a_->size();
    if (type_ == Type::Object) return o_->size();
    return 0;
}

// ---- Parser -----------------------------------------------------------------

namespace {

struct Parser {
    const std::string& s;
    size_t i = 0;

    [[noreturn]] void fail(const char* what) {
        throw std::runtime_error(std::string("JSON: ") + what + " bei Zeichen " + std::to_string(i));
    }
    void ws() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i;
    }
    static void utf8(std::string& out, uint32_t c) {
        if (c < 0x80) {
            out += char(c);
        } else if (c < 0x800) {
            out += char(0xC0 | (c >> 6));
            out += char(0x80 | (c & 0x3F));
        } else if (c < 0x10000) {
            out += char(0xE0 | (c >> 12));
            out += char(0x80 | ((c >> 6) & 0x3F));
            out += char(0x80 | (c & 0x3F));
        } else {
            out += char(0xF0 | (c >> 18));
            out += char(0x80 | ((c >> 12) & 0x3F));
            out += char(0x80 | ((c >> 6) & 0x3F));
            out += char(0x80 | (c & 0x3F));
        }
    }
    uint32_t hex4() {
        if (i + 4 > s.size()) fail("kurzes \\u");
        uint32_t v = uint32_t(std::strtoul(s.substr(i, 4).c_str(), nullptr, 16));
        i += 4;
        return v;
    }
    std::string string() {
        std::string out;
        ++i;   // "
        while (true) {
            if (i >= s.size()) fail("offener String");
            char c = s[i++];
            if (c == '"') return out;
            if (c != '\\') {
                out += c;
                continue;
            }
            char e = s[i++];
            switch (e) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'u': {
                    uint32_t c1 = hex4();
                    if (c1 >= 0xD800 && c1 < 0xDC00 && i + 1 < s.size() && s[i] == '\\' && s[i + 1] == 'u') {
                        i += 2;
                        uint32_t c2 = hex4();
                        c1 = 0x10000 + ((c1 - 0xD800) << 10) + (c2 - 0xDC00);
                    }
                    utf8(out, c1);
                    break;
                }
                default: out += e;
            }
        }
    }
    Value value() {
        ws();
        if (i >= s.size()) fail("Ende");
        char c = s[i];
        if (c == '{') {
            ++i;
            Object o;
            ws();
            if (s[i] == '}') {
                ++i;
                return Value(std::move(o));
            }
            while (true) {
                ws();
                if (s[i] != '"') fail("Schlüssel erwartet");
                std::string k = string();
                ws();
                if (s[i] != ':') fail(": erwartet");
                ++i;
                o.items.emplace_back(std::move(k), value());
                ws();
                if (s[i] == ',') {
                    ++i;
                    continue;
                }
                if (s[i] == '}') {
                    ++i;
                    return Value(std::move(o));
                }
                fail(", oder } erwartet");
            }
        }
        if (c == '[') {
            ++i;
            Array a;
            ws();
            if (s[i] == ']') {
                ++i;
                return Value(std::move(a));
            }
            while (true) {
                a.push_back(value());
                ws();
                if (s[i] == ',') {
                    ++i;
                    continue;
                }
                if (s[i] == ']') {
                    ++i;
                    return Value(std::move(a));
                }
                fail(", oder ] erwartet");
            }
        }
        if (c == '"') return Value(string());
        if (s.compare(i, 4, "true") == 0) {
            i += 4;
            return Value(true);
        }
        if (s.compare(i, 5, "false") == 0) {
            i += 5;
            return Value(false);
        }
        if (s.compare(i, 4, "null") == 0) {
            i += 4;
            return Value();
        }
        size_t st = i;
        bool isFloat = false;
        while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '-' || s[i] == '+' || s[i] == '.' ||
                                s[i] == 'e' || s[i] == 'E')) {
            if (s[i] == '.' || s[i] == 'e' || s[i] == 'E') isFloat = true;
            ++i;
        }
        if (st == i) fail("unerwartetes Zeichen");
        std::string num = s.substr(st, i - st);
        if (!isFloat) return Value(int64_t(std::strtoll(num.c_str(), nullptr, 10)));
        return Value(std::strtod(num.c_str(), nullptr));
    }
};

void writeValue(std::string& out, const Value& v, bool pretty, int depth) {
    auto nl = [&](int d) {
        if (pretty) {
            out += '\n';
            out.append(size_t(d) * 2, ' ');
        }
    };
    switch (v.type()) {
        case Value::Type::Null: out += "null"; break;
        case Value::Type::Bool: out += v.asBool() ? "true" : "false"; break;
        case Value::Type::Number: {
            double d = v.asDouble();
            if (std::isfinite(d) && d == std::floor(d) && std::fabs(d) < 9e15) {
                out += std::to_string(v.asInt());
            } else {
                char buf[64];
                std::snprintf(buf, sizeof buf, "%.17g", std::isfinite(d) ? d : 0.0);
                out += buf;
            }
            break;
        }
        case Value::Type::String: out += quote(v.asString()); break;
        case Value::Type::Array: {
            out += '[';
            bool first = true;
            for (const auto& e : v.asArray()) {
                if (!first) out += ',';
                first = false;
                nl(depth + 1);
                writeValue(out, e, pretty, depth + 1);
            }
            if (!v.asArray().empty()) nl(depth);
            out += ']';
            break;
        }
        case Value::Type::Object: {
            out += '{';
            bool first = true;
            for (const auto& [k, e] : v.asObject()) {
                if (!first) out += ',';
                first = false;
                nl(depth + 1);
                out += quote(k);
                out += pretty ? ": " : ":";
                writeValue(out, e, pretty, depth + 1);
            }
            if (v.asObject().size()) nl(depth);
            out += '}';
            break;
        }
    }
}

}  // namespace

Value parse(const std::string& text) {
    Parser p{text};
    // UTF-8-BOM überspringen (Windows-Editoren)
    if (text.size() >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF)
        p.i = 3;
    return p.value();
}

std::string quote(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += char(c);
                }
        }
    }
    return out + "\"";
}

std::string write(const Value& v, bool pretty) {
    std::string out;
    writeValue(out, v, pretty, 0);
    return out;
}

}  // namespace json
