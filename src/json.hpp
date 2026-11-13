// Kleiner JSON-Leser/-Schreiber (Reihenfolge der Schlüssel bleibt erhalten).
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace json {

class Value;
using Array = std::vector<Value>;

class Object {
public:
    std::vector<std::pair<std::string, Value>> items;
    const Value* find(const std::string& key) const;
    Value* find(const std::string& key);
    Value& operator[](const std::string& key);
    bool contains(const std::string& key) const { return find(key) != nullptr; }
    size_t size() const { return items.size(); }
    auto begin() const { return items.begin(); }
    auto end() const { return items.end(); }

private:
    mutable std::unordered_map<std::string, size_t> index_;
    mutable size_t indexed_ = 0;
    void reindex() const;
};

class Value {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Value() = default;
    Value(std::nullptr_t) {}
    Value(bool b) : type_(Type::Bool), b_(b) {}
    Value(int v) : type_(Type::Number), n_(v), isInt_(true), i_(v) {}
    Value(int64_t v) : type_(Type::Number), n_(double(v)), isInt_(true), i_(v) {}
    Value(double v) : type_(Type::Number), n_(v) {}
    Value(const char* s) : type_(Type::String), s_(std::make_shared<std::string>(s)) {}
    Value(std::string s) : type_(Type::String), s_(std::make_shared<std::string>(std::move(s))) {}
    Value(Array a) : type_(Type::Array), a_(std::make_shared<Array>(std::move(a))) {}
    Value(Object o) : type_(Type::Object), o_(std::make_shared<Object>(std::move(o))) {}

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Bool; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }

    bool asBool(bool def = false) const { return type_ == Type::Bool ? b_ : def; }
    double asDouble(double def = 0) const { return type_ == Type::Number ? n_ : def; }
    int64_t asInt(int64_t def = 0) const { return type_ == Type::Number ? (isInt_ ? i_ : int64_t(n_)) : def; }
    const std::string& asString() const;
    std::string str(const std::string& def = "") const { return type_ == Type::String ? *s_ : def; }
    const Array& asArray() const;
    Array& asArray();
    const Object& asObject() const;
    Object& asObject();

    // Bequemer Zugriff: v["x"] (fehlend -> null), v[3]
    const Value& operator[](const std::string& key) const;
    const Value& operator[](size_t i) const;
    size_t size() const;

private:
    Type type_ = Type::Null;
    bool b_ = false;
    double n_ = 0;
    bool isInt_ = false;
    int64_t i_ = 0;
    std::shared_ptr<std::string> s_;
    std::shared_ptr<Array> a_;
    std::shared_ptr<Object> o_;
};

Value parse(const std::string& text);
std::string write(const Value& v, bool pretty = false);
std::string quote(const std::string& s);

}  // namespace json
