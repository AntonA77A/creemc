#include "auth.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "json.hpp"

// Plattform-Teil: Windows nutzt CNG + WinHTTP, Linux OpenSSL + eigene HTTP(S)-Anfrage.
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <winhttp.h>
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "winhttp.lib")
#else
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace auth {

namespace {
struct Url {
    bool https = true;
    std::string host, path;
    int port = 443;
};

Url parseUrl(const std::string& url) {
    Url u;
    std::string rest = url;
    if (rest.rfind("https://", 0) == 0) {
        rest = rest.substr(8);
    } else if (rest.rfind("http://", 0) == 0) {
        rest = rest.substr(7);
        u.https = false;
        u.port = 80;
    }
    size_t slash = rest.find('/');
    std::string hostPort = slash == std::string::npos ? rest : rest.substr(0, slash);
    u.path = slash == std::string::npos ? "/" : rest.substr(slash);
    size_t colon = hostPort.rfind(':');
    if (colon != std::string::npos) {
        u.host = hostPort.substr(0, colon);
        u.port = std::atoi(hostPort.c_str() + colon + 1);
    } else {
        u.host = hostPort;
    }
    return u;
}

std::string urlEncode(const std::string& s) {
    std::string out;
    static const char* hx = "0123456789ABCDEF";
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += char(c);
        } else {
            out += '%';
            out += hx[c >> 4];
            out += hx[c & 15];
        }
    }
    return out;
}
}  // namespace

#ifdef _WIN32
// =============================== Windows =====================================

static void check(NTSTATUS st, const char* what) {
    if (st < 0) throw std::runtime_error(std::string("Krypto-Fehler: ") + what);
}

// ASN.1/DER für den öffentlichen Schlüssel (SubjectPublicKeyInfo)
static Bytes derLen(size_t n) {
    Bytes b;
    if (n < 0x80) {
        b += char(n);
    } else if (n < 0x100) {
        b += char(0x81);
        b += char(n);
    } else {
        b += char(0x82);
        b += char(n >> 8);
        b += char(n & 0xFF);
    }
    return b;
}
static Bytes der(uint8_t tag, const Bytes& body) { return Bytes(1, char(tag)) + derLen(body.size()) + body; }
static Bytes derInt(Bytes v) {
    while (v.size() > 1 && v[0] == 0 && !(uint8_t(v[1]) & 0x80)) v.erase(0, 1);
    if (uint8_t(v[0]) & 0x80) v.insert(v.begin(), '\0');
    return der(0x02, v);
}

KeyPair::KeyPair() {
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    check(BCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_ALGORITHM, nullptr, 0), "RSA");
    check(BCryptGenerateKeyPair(alg, &key, 1024, 0), "Schlüssel erzeugen");
    check(BCryptFinalizeKeyPair(key, 0), "Schlüssel fertigstellen");
    ULONG size = 0;
    check(BCryptExportKey(key, nullptr, BCRYPT_RSAPUBLIC_BLOB, nullptr, 0, &size, 0), "Export");
    std::vector<unsigned char> blob(size);
    check(BCryptExportKey(key, nullptr, BCRYPT_RSAPUBLIC_BLOB, blob.data(), size, &size, 0), "Export");
    auto* hdr = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(blob.data());
    const char* p = reinterpret_cast<const char*>(blob.data()) + sizeof(BCRYPT_RSAKEY_BLOB);
    Bytes exponent(p, hdr->cbPublicExp);
    Bytes modulus(p + hdr->cbPublicExp, hdr->cbModulus);
    Bytes algId = der(0x30, Bytes("\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01", 11) + Bytes("\x05\x00", 2));
    Bytes rsaKey = der(0x30, derInt(modulus) + derInt(exponent));
    publicDer = der(0x30, algId + der(0x03, Bytes(1, '\0') + rsaKey));
    alg_ = alg;
    key_ = key;
}

KeyPair::~KeyPair() {
    if (key_) BCryptDestroyKey(key_);
    if (alg_) BCryptCloseAlgorithmProvider(alg_, 0);
}

bool KeyPair::decrypt(const Bytes& in, Bytes& out) const {
    ULONG size = 0;
    auto* d = reinterpret_cast<PUCHAR>(const_cast<char*>(in.data()));
    if (BCryptDecrypt(key_, d, ULONG(in.size()), nullptr, nullptr, 0, nullptr, 0, &size, BCRYPT_PAD_PKCS1) < 0) return false;
    out.assign(size, '\0');
    if (BCryptDecrypt(key_, d, ULONG(in.size()), nullptr, nullptr, 0, reinterpret_cast<PUCHAR>(out.data()), size, &size,
                      BCRYPT_PAD_PKCS1) < 0)
        return false;
    out.resize(size);
    return true;
}

Cipher::Cipher(const Bytes& secret, bool encrypt) : encrypt_(encrypt) {
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    check(BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, nullptr, 0), "AES");
    check(BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CFB, ULONG(sizeof(BCRYPT_CHAIN_MODE_CFB)), 0), "CFB");
    check(BCryptGenerateSymmetricKey(alg, &key, nullptr, 0, (PUCHAR)secret.data(), ULONG(secret.size()), 0), "AES-Schlüssel");
    DWORD blockLen = 1;   // CFB8: 1 Byte Rückkopplung
    check(BCryptSetProperty(key, BCRYPT_MESSAGE_BLOCK_LENGTH, (PUCHAR)&blockLen, sizeof(blockLen), 0), "CFB8");
    std::memcpy(iv_, secret.data(), 16);
    alg_ = alg;
    key_ = key;
}

Cipher::~Cipher() {
    if (key_) BCryptDestroyKey(key_);
    if (alg_) BCryptCloseAlgorithmProvider(alg_, 0);
}

void Cipher::apply(char* data, size_t len) {
    if (!len) return;
    ULONG done = 0;
    auto* p = reinterpret_cast<PUCHAR>(data);
    NTSTATUS st = encrypt_ ? BCryptEncrypt(key_, p, ULONG(len), nullptr, iv_, 16, p, ULONG(len), &done, 0)
                           : BCryptDecrypt(key_, p, ULONG(len), nullptr, iv_, 16, p, ULONG(len), &done, 0);
    check(st, "AES/CFB8");
}

static Bytes hashWith(LPCWSTR algName, const Bytes& in) {
    BCRYPT_ALG_HANDLE alg;
    check(BCryptOpenAlgorithmProvider(&alg, algName, nullptr, 0), "Hash");
    DWORD len = 0, got = 0;
    BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, (PUCHAR)&len, sizeof(len), &got, 0);
    Bytes out(len, '\0');
    NTSTATUS st = BCryptHash(alg, nullptr, 0, (PUCHAR)in.data(), ULONG(in.size()), (PUCHAR)out.data(), len);
    BCryptCloseAlgorithmProvider(alg, 0);
    check(st, "Hash");
    return out;
}

static Bytes sha1(const Bytes& d) { return hashWith(BCRYPT_SHA1_ALGORITHM, d); }
Bytes md5(const Bytes& d) { return hashWith(BCRYPT_MD5_ALGORITHM, d); }

Bytes randomBytes(size_t n) {
    Bytes out(n, '\0');
    BCryptGenRandom(nullptr, (PUCHAR)out.data(), ULONG(n), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return out;
}

static std::wstring widen(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    std::wstring w(size_t(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), w.data(), n);
    return w;
}

// -> (HTTP-Status, Body); wirft bei Verbindungsfehlern
static std::pair<int, std::string> httpGet(const std::string& url) {
    Url u = parseUrl(url);
    HINTERNET s = WinHttpOpen(L"CreeMC", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) throw std::runtime_error("WinHTTP");
    WinHttpSetTimeouts(s, 10000, 10000, 10000, 10000);
    HINTERNET c = WinHttpConnect(s, widen(u.host).c_str(), INTERNET_PORT(u.port), 0);
    HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", widen(u.path).c_str(), nullptr, WINHTTP_NO_REFERER,
                                         WINHTTP_DEFAULT_ACCEPT_TYPES, u.https ? WINHTTP_FLAG_SECURE : 0)
                    : nullptr;
    auto cleanup = [&] {
        if (r) WinHttpCloseHandle(r);
        if (c) WinHttpCloseHandle(c);
        WinHttpCloseHandle(s);
    };
    if (!r || !WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(r, nullptr)) {
        cleanup();
        throw std::runtime_error("Session-Server nicht erreichbar");
    }
    DWORD status = 0, size = sizeof(status);
    WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &size, nullptr);
    std::string body;
    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(r, &avail) || !avail) break;
        std::string buf(avail, '\0');
        DWORD got = 0;
        if (!WinHttpReadData(r, buf.data(), avail, &got)) break;
        body.append(buf.data(), got);
    }
    cleanup();
    return {int(status), body};
}

#else
// ================================ Linux ======================================

static void sslCheck(bool ok, const char* what) {
    if (!ok) throw std::runtime_error(std::string("Krypto-Fehler (OpenSSL): ") + what);
}

KeyPair::KeyPair() {
    EVP_PKEY* pkey = EVP_RSA_gen(1024);
    sslCheck(pkey != nullptr, "RSA-Schlüssel erzeugen");
    unsigned char* der = nullptr;
    int len = i2d_PUBKEY(pkey, &der);   // SubjectPublicKeyInfo
    sslCheck(len > 0, "Schlüssel exportieren");
    publicDer.assign(reinterpret_cast<char*>(der), size_t(len));
    OPENSSL_free(der);
    key_ = pkey;
}

KeyPair::~KeyPair() {
    if (key_) EVP_PKEY_free(static_cast<EVP_PKEY*>(key_));
}

bool KeyPair::decrypt(const Bytes& in, Bytes& out) const {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(static_cast<EVP_PKEY*>(key_), nullptr);
    if (!ctx) return false;
    bool ok = false;
    size_t len = 0;
    const auto* d = reinterpret_cast<const unsigned char*>(in.data());
    if (EVP_PKEY_decrypt_init(ctx) > 0 && EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) > 0 &&
        EVP_PKEY_decrypt(ctx, nullptr, &len, d, in.size()) > 0) {
        out.assign(len, '\0');
        if (EVP_PKEY_decrypt(ctx, reinterpret_cast<unsigned char*>(out.data()), &len, d, in.size()) > 0) {
            out.resize(len);
            ok = true;
        }
    }
    EVP_PKEY_CTX_free(ctx);
    return ok;
}

Cipher::Cipher(const Bytes& secret, bool encrypt) : encrypt_(encrypt) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    sslCheck(ctx != nullptr, "AES");
    const auto* k = reinterpret_cast<const unsigned char*>(secret.data());
    sslCheck(EVP_CipherInit_ex(ctx, EVP_aes_128_cfb8(), nullptr, k, k, encrypt ? 1 : 0) == 1, "AES/CFB8");
    key_ = ctx;
}

Cipher::~Cipher() {
    if (key_) EVP_CIPHER_CTX_free(static_cast<EVP_CIPHER_CTX*>(key_));
}

void Cipher::apply(char* data, size_t len) {
    if (!len) return;
    int out = 0;
    auto* p = reinterpret_cast<unsigned char*>(data);
    sslCheck(EVP_CipherUpdate(static_cast<EVP_CIPHER_CTX*>(key_), p, &out, p, int(len)) == 1, "AES/CFB8");
}

static Bytes digest(const EVP_MD* md, const Bytes& in) {
    unsigned char buf[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    sslCheck(EVP_Digest(in.data(), in.size(), buf, &len, md, nullptr) == 1, "Hash");
    return Bytes(reinterpret_cast<char*>(buf), len);
}

static Bytes sha1(const Bytes& d) { return digest(EVP_sha1(), d); }
Bytes md5(const Bytes& d) { return digest(EVP_md5(), d); }

Bytes randomBytes(size_t n) {
    Bytes out(n, '\0');
    RAND_bytes(reinterpret_cast<unsigned char*>(out.data()), int(n));
    return out;
}

// Zertifikate: übliche Orte der Linux-Distributionen (und Pterodactyl-Container)
static void loadCaCerts(SSL_CTX* ctx) {
    SSL_CTX_set_default_verify_paths(ctx);
    for (const char* f : {"/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt", "/etc/ssl/cert.pem",
                          "/etc/ssl/ca-bundle.pem", "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem"})
        if (access(f, R_OK) == 0) SSL_CTX_load_verify_locations(ctx, f, nullptr);
    if (access("/etc/ssl/certs", R_OK) == 0) SSL_CTX_load_verify_locations(ctx, nullptr, "/etc/ssl/certs");
}

static std::string dechunk(const std::string& body) {
    std::string out;
    size_t i = 0;
    while (i < body.size()) {
        size_t nl = body.find("\r\n", i);
        if (nl == std::string::npos) break;
        size_t n = std::strtoul(body.substr(i, nl - i).c_str(), nullptr, 16);
        if (n == 0) break;
        out += body.substr(nl + 2, n);
        i = nl + 2 + n + 2;
    }
    return out;
}

static std::pair<int, std::string> httpGet(const std::string& url) {
    Url u = parseUrl(url);
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(u.host.c_str(), std::to_string(u.port).c_str(), &hints, &res) != 0 || !res)
        throw std::runtime_error("Session-Server nicht erreichbar (DNS)");
    int fd = -1;
    for (addrinfo* a = res; a; a = a->ai_next) {
        fd = ::socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (fd < 0) continue;
        timeval tv{10, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
        if (::connect(fd, a->ai_addr, a->ai_addrlen) == 0) break;
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) throw std::runtime_error("Session-Server nicht erreichbar");

    std::string request = "GET " + u.path + " HTTP/1.1\r\nHost: " + u.host + "\r\nUser-Agent: CreeMC\r\nConnection: close\r\n\r\n";
    std::string raw;
    char buf[16384];
    if (u.https) {
        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        loadCaCerts(ctx);
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, fd);
        SSL_set_tlsext_host_name(ssl, u.host.c_str());
        SSL_set1_host(ssl, u.host.c_str());
        bool ok = SSL_connect(ssl) == 1 && SSL_write(ssl, request.data(), int(request.size())) > 0;
        if (ok) {
            int n;
            while ((n = SSL_read(ssl, buf, sizeof buf)) > 0) raw.append(buf, size_t(n));
        }
        long verify = SSL_get_verify_result(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        ::close(fd);
        if (!ok) {
            throw std::runtime_error(verify != X509_V_OK ? "TLS-Zertifikat nicht prüfbar (ca-certificates installiert?)"
                                                         : "TLS-Verbindung fehlgeschlagen");
        }
    } else {
        ::send(fd, request.data(), request.size(), MSG_NOSIGNAL);
        ssize_t n;
        while ((n = ::recv(fd, buf, sizeof buf, 0)) > 0) raw.append(buf, size_t(n));
        ::close(fd);
    }
    size_t headEnd = raw.find("\r\n\r\n");
    if (headEnd == std::string::npos) throw std::runtime_error("ungültige HTTP-Antwort");
    std::string head = raw.substr(0, headEnd), body = raw.substr(headEnd + 4);
    int status = 0;
    size_t sp = head.find(' ');
    if (sp != std::string::npos) status = std::atoi(head.c_str() + sp + 1);
    std::string lower = head;
    for (auto& ch : lower) ch = char(std::tolower((unsigned char)ch));
    if (lower.find("transfer-encoding: chunked") != std::string::npos) body = dechunk(body);
    return {status, body};
}
#endif

// ============================ gemeinsam ======================================

std::string serverHash(const std::string& serverId, const Bytes& secret, const Bytes& publicDer) {
    Bytes d = sha1(serverId + secret + publicDer);
    bool negative = uint8_t(d[0]) & 0x80;
    if (negative) {   // Zweierkomplement -> Betrag (Minecraft: vorzeichenbehaftete Hexzahl)
        for (auto& c : d) c = char(~uint8_t(c));
        for (int i = int(d.size()) - 1; i >= 0; --i) {
            d[size_t(i)] = char(uint8_t(d[size_t(i)]) + 1);
            if (d[size_t(i)] != 0) break;
        }
    }
    static const char* hx = "0123456789abcdef";
    std::string s;
    for (unsigned char c : d) {
        s += hx[c >> 4];
        s += hx[c & 15];
    }
    size_t nz = s.find_first_not_of('0');
    s = nz == std::string::npos ? "0" : s.substr(nz);
    return negative ? "-" + s : s;
}

std::optional<Profile> hasJoined(const std::string& name, const std::string& hash) {
    const char* env = std::getenv("CREEMC_SESSION_SERVER");   // zum Testen umleitbar
    std::string base = env ? env : "https://sessionserver.mojang.com";
    auto [status, body] = httpGet(base + "/session/minecraft/hasJoined?username=" + urlEncode(name) + "&serverId=" + urlEncode(hash));
    if (status == 204 || body.empty()) return std::nullopt;
    if (status != 200) throw std::runtime_error("Session-Server antwortet mit " + std::to_string(status));
    json::Value v = json::parse(body);
    if (!v["id"].isString()) return std::nullopt;
    Profile p;
    p.uuid = Uuid::parse(v["id"].asString());
    p.name = v["name"].asString();
    for (const auto& pr : v["properties"].asArray())
        p.properties.push_back({pr["name"].asString(), pr["value"].asString(), pr["signature"].str()});
    return p;
}

}  // namespace auth
