#!/bin/ash
# CreeMC installieren (läuft im Pterodactyl-Installationscontainer, Alpine Linux)
#  1. DOWNLOAD_URL gesetzt  -> fertige Linux-Datei herunterladen
#  2. sonst: Quellcode bauen -> aus SOURCE_URL oder aus der hochgeladenen creemc-source.tar.gz
set -e
cd /mnt/server
mkdir -p plugins

if [ -n "${DOWNLOAD_URL}" ]; then
    apk add --no-cache curl >/dev/null
    echo "Lade fertiges CreeMC von ${DOWNLOAD_URL} ..."
    curl -fsSL -o creemc "${DOWNLOAD_URL}"
else
    echo "Installiere Compiler und Bibliotheken ..."
    apk add --no-cache build-base cmake openssl-dev openssl-libs-static linux-headers curl tar >/dev/null
    SRC=/tmp/creemc-src
    rm -rf "${SRC}" /tmp/creemc-build
    mkdir -p "${SRC}"
    if [ -n "${SOURCE_URL}" ]; then
        echo "Lade Quellcode von ${SOURCE_URL} ..."
        curl -fsSL -o /tmp/creemc-source.tar.gz "${SOURCE_URL}"
        ARCHIVE=/tmp/creemc-source.tar.gz
    elif [ -f /mnt/server/creemc-source.tar.gz ]; then
        ARCHIVE=/mnt/server/creemc-source.tar.gz
    else
        echo "FEHLER: Kein Quellcode gefunden."
        echo "Lade 'creemc-source.tar.gz' per Dateimanager in den Server-Ordner hoch und installiere neu,"
        echo "oder trage eine Quellcode- bzw. Download-URL in den Server-Einstellungen ein."
        exit 1
    fi
    tar -xzf "${ARCHIVE}" -C "${SRC}"
    CMAKE_FILE=$(grep -l "project(creemc" $(find "${SRC}" -name CMakeLists.txt) | head -n 1)
    if [ -z "${CMAKE_FILE}" ]; then
        echo "FEHLER: Im Archiv ist kein CreeMC-Quellcode (CMakeLists.txt fehlt)."
        exit 1
    fi
    DIR=$(dirname "${CMAKE_FILE}")
    echo "Baue CreeMC (das dauert ein bis zwei Minuten) ..."
    cmake -S "${DIR}" -B /tmp/creemc-build -DCMAKE_BUILD_TYPE=Release -DCREEMC_STATIC=ON
    cmake --build /tmp/creemc-build -j 2
    strip /tmp/creemc-build/creemc
    cp /tmp/creemc-build/creemc ./creemc
    rm -rf "${SRC}" /tmp/creemc-build
fi
chmod +x creemc

if [ ! -f config.toml ]; then
cat > config.toml <<EOF
# CreeMC Konfiguration (Port, Spielerzahl, Online-Modus und Sichtweite stellt das Panel ein)
host = "0.0.0.0"
port = ${SERVER_PORT:-25565}
motd = "&a&lCreeMC &7- &fC++-Minecraft-Server"
max_players = ${MAX_PLAYERS:-20}
view_distance = ${VIEW_DISTANCE:-8}
gamemode = 0
difficulty = 1
world_name = "world"
generator = "normal"
seed = 12345
daylight_cycle = true
chat_format = "&7{name}&8: &f{message}"
join_message = "&e{name} hat den Server betreten"
quit_message = "&e{name} hat den Server verlassen"
online_mode = ${ONLINE_MODE:-true}
EOF
fi
echo "Installation fertig."
