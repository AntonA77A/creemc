#!/bin/sh
# Baut Beispiel.jar (braucht Java 21+ JDK). Danach Beispiel.jar in den plugins/-Ordner des Servers legen.
set -e
cd "$(dirname "$0")"
API=creemc-api.jar
[ -f "$API" ] || API=../dist/creemc-api.jar
rm -rf classes
javac --release 21 -encoding UTF-8 -cp "$API" -d classes $(find src -name '*.java')
cp -r res/* classes/
jar --create --file Beispiel.jar -C classes .
echo "Fertig: Beispiel.jar"
