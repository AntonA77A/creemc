// Online-Modus: RSA-Schlüssel, AES/CFB8-Verschlüsselung und Mojang-Session-Server (Windows CNG + WinHTTP).
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "util.hpp"
#include "version.hpp"

namespace auth {

class KeyPair {
public:
    KeyPair();
    ~KeyPair();
    Bytes publicDer;
    bool decrypt(const Bytes& in, Bytes& out) const;   // RSA PKCS#1 v1.5

private:
    void* alg_ = nullptr;
    void* key_ = nullptr;
};

// AES-128/CFB8, Schlüssel = IV = gemeinsames Geheimnis; ein Objekt pro Richtung
class Cipher {
public:
    Cipher(const Bytes& secret, bool encrypt);
    ~Cipher();
    void apply(char* data, size_t len);

private:
    void* alg_ = nullptr;
    void* key_ = nullptr;
    unsigned char iv_[16];
    bool encrypt_;
};

std::string serverHash(const std::string& serverId, const Bytes& secret, const Bytes& publicDer);
Bytes md5(const Bytes& data);
Bytes randomBytes(size_t n);

struct Profile {
    Uuid uuid;
    std::string name;
    std::vector<ProfileProperty> properties;
};

// -> Profil, std::nullopt = nicht angemeldet; wirft bei Netzwerkfehlern
std::optional<Profile> hasJoined(const std::string& name, const std::string& hash);

}  // namespace auth
