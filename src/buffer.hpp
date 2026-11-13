// Minecraft-Datentypen lesen und schreiben (VarInt, Strings, Positionen ...).
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include "util.hpp"

struct ProtocolError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// ---- Lesen ------------------------------------------------------------------
class Reader {
public:
    explicit Reader(const Bytes& data) : d_(data) {}

    size_t remaining() const { return d_.size() - pos_; }
    Bytes read(size_t n) {
        need(n);
        Bytes b = d_.substr(pos_, n);
        pos_ += n;
        return b;
    }
    Bytes rest() {
        Bytes b = d_.substr(pos_);
        pos_ = d_.size();
        return b;
    }
    uint8_t u8() {
        need(1);
        return uint8_t(d_[pos_++]);
    }
    int8_t i8() { return int8_t(u8()); }
    bool boolean() { return u8() != 0; }
    uint16_t u16() { return uint16_t(be(2)); }
    int16_t i16() { return int16_t(be(2)); }
    int32_t i32() { return int32_t(be(4)); }
    int64_t i64() { return int64_t(be(8)); }
    float f32() {
        uint32_t u = uint32_t(be(4));
        float f;
        std::memcpy(&f, &u, 4);
        return f;
    }
    double f64() {
        uint64_t u = be(8);
        double f;
        std::memcpy(&f, &u, 8);
        return f;
    }
    int32_t varint() {
        uint32_t n = 0;
        for (int i = 0; i < 5; ++i) {
            uint8_t b = u8();
            n |= uint32_t(b & 0x7F) << (7 * i);
            if (!(b & 0x80)) return int32_t(n);
        }
        throw ProtocolError("VarInt zu groß");
    }
    int64_t varlong() {
        uint64_t n = 0;
        for (int i = 0; i < 10; ++i) {
            uint8_t b = u8();
            n |= uint64_t(b & 0x7F) << (7 * i);
            if (!(b & 0x80)) return int64_t(n);
        }
        throw ProtocolError("VarLong zu groß");
    }
    std::string string(int maxLen = 32767) {
        int32_t n = varint();
        if (n < 0 || n > maxLen * 3) throw ProtocolError("String zu lang");
        return read(size_t(n));
    }
    Uuid uuid() {
        Uuid u;
        Bytes b = read(16);
        std::memcpy(u.b.data(), b.data(), 16);
        return u;
    }
    void position(int& x, int& y, int& z) {
        int64_t v = i64();
        x = int(v >> 38);
        y = int(v << 52 >> 52);
        z = int(v << 26 >> 38);
    }

private:
    void need(size_t n) const {
        if (pos_ + n > d_.size()) throw ProtocolError("Paket zu kurz");
    }
    uint64_t be(int n) {
        need(size_t(n));
        uint64_t v = 0;
        for (int i = 0; i < n; ++i) v = (v << 8) | uint8_t(d_[pos_++]);
        return v;
    }
    const Bytes& d_;
    size_t pos_ = 0;
};

// ---- Schreiben --------------------------------------------------------------
class Writer {
public:
    Bytes out;

    Writer& u8(uint8_t v) {
        out += char(v);
        return *this;
    }
    Writer& i8(int8_t v) { return u8(uint8_t(v)); }
    Writer& boolean(bool v) { return u8(v ? 1 : 0); }
    Writer& u16(uint16_t v) { return be(v, 2); }
    Writer& i16(int16_t v) { return be(uint16_t(v), 2); }
    Writer& i32(int32_t v) { return be(uint32_t(v), 4); }
    Writer& u32(uint32_t v) { return be(v, 4); }
    Writer& i64(int64_t v) { return be(uint64_t(v), 8); }
    Writer& f32(float f) {
        uint32_t u;
        std::memcpy(&u, &f, 4);
        return be(u, 4);
    }
    Writer& f64(double f) {
        uint64_t u;
        std::memcpy(&u, &f, 8);
        return be(u, 8);
    }
    Writer& varint(int32_t v) {
        uint32_t n = uint32_t(v);
        while (n & ~0x7Fu) {
            out += char((n & 0x7F) | 0x80);
            n >>= 7;
        }
        out += char(n);
        return *this;
    }
    Writer& varlong(int64_t v) {
        uint64_t n = uint64_t(v);
        while (n & ~0x7Full) {
            out += char((n & 0x7F) | 0x80);
            n >>= 7;
        }
        out += char(n);
        return *this;
    }
    Writer& string(const std::string& s) {
        varint(int32_t(s.size()));
        out += s;
        return *this;
    }
    Writer& raw(const Bytes& b) {
        out += b;
        return *this;
    }
    Writer& prefixed(const Bytes& b) {
        varint(int32_t(b.size()));
        out += b;
        return *this;
    }
    Writer& uuid(const Uuid& u) {
        out.append(reinterpret_cast<const char*>(u.b.data()), 16);
        return *this;
    }
    Writer& position(int x, int y, int z) {
        uint64_t v = (uint64_t(int64_t(x) & 0x3FFFFFF) << 38) | (uint64_t(int64_t(z) & 0x3FFFFFF) << 12) |
                     uint64_t(int64_t(y) & 0xFFF);
        return be(v, 8);
    }
    Writer& angle(double deg) {
        double m = std::fmod(deg, 360.0);
        if (m < 0) m += 360.0;
        return u8(uint8_t(int(m / 360.0 * 256.0) & 0xFF));
    }

private:
    Writer& be(uint64_t v, int n) {
        for (int i = n - 1; i >= 0; --i) out += char((v >> (8 * i)) & 0xFF);
        return *this;
    }
};

inline Bytes varintBytes(int32_t v) {
    Writer w;
    w.varint(v);
    return w.out;
}
