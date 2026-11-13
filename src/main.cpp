// CreeMC (C++) - Minecraft-Server für 1.21 bis 26.3.
#include <exception>
#include <iostream>

#include "data.hpp"
#include "server.hpp"
#include "util.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    std::signal(SIGPIPE, SIG_IGN);   // getrennte Clients sollen den Server nicht beenden
#endif
    std::string root = argc > 1 ? argv[1] : ".";
    try {
        data::init();
        Server server(root);
        return server.run();
    } catch (const std::exception& e) {
        logx::error(std::string("Start fehlgeschlagen: ") + e.what());
        return 1;
    }
}
