#include "nbt.hpp"

namespace nbt {

std::string mutf8(const std::string& s) {
    bool simple = true;
    for (unsigned char c : s)
        if (c == 0 || c >= 0x80) simple = false;
    if (simple) return s;
    // UTF-8 -> Codepoints -> UTF-16 -> modifiziertes UTF-8
    std::string out;
    auto put16 = [&](uint32_t c) {
        if (c == 0) {
            out += char(0xC0);
            out += char(0x80);
        } else if (c < 0x80) {
            out += char(c);
        } else if (c < 0x800) {
            out += char(0xC0 | (c >> 6));
            out += char(0x80 | (c & 0x3F));
        } else {
            out += char(0xE0 | (c >> 12));
            out += char(0x80 | ((c >> 6) & 0x3F));
            out += char(0x80 | (c & 0x3F));
        }
    };
    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        uint32_t cp;
        int len;
        if (c < 0x80) { cp = c; len = 1; }
        else if ((c >> 5) == 6) { cp = c & 0x1F; len = 2; }
        else if ((c >> 4) == 14) { cp = c & 0x0F; len = 3; }
        else { cp = c & 0x07; len = 4; }
        for (int k = 1; k < len && i + k < s.size(); ++k) cp = (cp << 6) | ((unsigned char)s[i + k] & 0x3F);
        i += size_t(len);
        if (cp >= 0x10000) {
            cp -= 0x10000;
            put16(0xD800 + (cp >> 10));
            put16(0xDC00 + (cp & 0x3FF));
        } else {
            put16(cp);
        }
    }
    return out;
}

void Tag::writePayload(Writer& w) const {
    switch (type_) {
        case End: break;
        case Byte: w.i8(int8_t(i_)); break;
        case Short: w.i16(int16_t(i_)); break;
        case Int: w.i32(int32_t(i_)); break;
        case Long: w.i64(i_); break;
        case Float: w.f32(float(d_)); break;
        case Double: w.f64(d_); break;
        case ByteArray: w.i32(int32_t(s_.size())).raw(s_); break;
        case String: {
            std::string m = mutf8(s_);
            w.u16(uint16_t(m.size())).raw(m);
            break;
        }
        case List: {
            if (items_.empty()) {
                w.u8(End).i32(0);
                break;
            }
            w.u8(items_[0].type_).i32(int32_t(items_.size()));
            for (const auto& t : items_) t.writePayload(w);
            break;
        }
        case Compound: {
            for (const auto& [k, t] : entries_) {
                std::string m = mutf8(k);
                w.u8(t.type_).u16(uint16_t(m.size())).raw(m);
                t.writePayload(w);
            }
            w.u8(End);
            break;
        }
        case IntArray: break;
        case LongArray: {
            w.i32(int32_t(longs_.size()));
            for (int64_t v : longs_) w.i64(v);
            break;
        }
    }
}

Bytes Tag::encode() const {
    Writer w;
    w.u8(type_);
    writePayload(w);
    return w.out;
}

}  // namespace nbt
