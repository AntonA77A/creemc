// Minimaler NBT-Schreiber (Netzwerkformat: namenloses Root-Tag).
#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "buffer.hpp"

namespace nbt {

class Tag {
public:
    enum Type : uint8_t { End = 0, Byte = 1, Short = 2, Int = 3, Long = 4, Float = 5, Double = 6, ByteArray = 7,
                          String = 8, List = 9, Compound = 10, IntArray = 11, LongArray = 12 };

    static Tag byte(int8_t v) { Tag t(Byte); t.i_ = v; return t; }
    static Tag boolean(bool v) { return byte(v ? 1 : 0); }
    static Tag shortv(int16_t v) { Tag t(Short); t.i_ = v; return t; }
    static Tag intv(int32_t v) { Tag t(Int); t.i_ = v; return t; }
    static Tag longv(int64_t v) { Tag t(Long); t.i_ = v; return t; }
    static Tag floatv(float v) { Tag t(Float); t.d_ = v; return t; }
    static Tag doublev(double v) { Tag t(Double); t.d_ = v; return t; }
    static Tag string(std::string v) { Tag t(String); t.s_ = std::move(v); return t; }
    static Tag compound() { return Tag(Compound); }
    static Tag list() { return Tag(List); }
    static Tag longArray(std::vector<int64_t> v) { Tag t(LongArray); t.longs_ = std::move(v); return t; }

    Tag& put(const std::string& key, Tag v) {
        entries_.emplace_back(key, std::move(v));
        return *this;
    }
    Tag& add(Tag v) {
        items_.push_back(std::move(v));
        return *this;
    }
    Type type() const { return type_; }

    void writePayload(Writer& w) const;
    Bytes encode() const;   // namenloses Root-Tag

private:
    explicit Tag(Type t) : type_(t) {}
    Type type_;
    int64_t i_ = 0;
    double d_ = 0;
    std::string s_;
    std::vector<std::pair<std::string, Tag>> entries_;
    std::vector<Tag> items_;
    std::vector<int64_t> longs_;
};

std::string mutf8(const std::string& utf8);   // Java "modified UTF-8"

}  // namespace nbt
