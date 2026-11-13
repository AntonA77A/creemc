#!/bin/sh
# Baut die Linux-Version direkt unter Linux (braucht g++, cmake, OpenSSL-Entwicklerdateien).
# Für eine komplett statische Datei: in Alpine bauen (siehe Dockerfile.linux) mit -DCREEMC_STATIC=ON.
set -e
cd "$(dirname "$0")/.."
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release ${CREEMC_STATIC:+-DCREEMC_STATIC=ON}
cmake --build build-linux -j
mkdir -p dist
cp build-linux/creemc dist/creemc-linux-x64
echo "Fertig: dist/creemc-linux-x64"
