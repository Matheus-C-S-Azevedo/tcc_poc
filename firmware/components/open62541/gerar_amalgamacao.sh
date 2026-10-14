#!/usr/bin/env bash
# Gera open62541.c / include/open62541.h a partir do código oficial.
# Uso: ./gerar_amalgamacao.sh [versão]   (padrão: v1.5.9)
# Requer: git, cmake, make, python3 e um compilador C do host.
set -euo pipefail

VERSAO="${1:-v1.5.9}"
DESTINO="$(cd "$(dirname "$0")" && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

git clone --quiet --depth 1 --branch "$VERSAO" \
    https://github.com/open62541/open62541.git "$TMP/open62541"

# O CMake do open62541 tenta compilar o kernel do FreeRTOS (simulador POSIX).
# Para gerar a amalgamação nada é compilado, então um kernel vazio basta.
mkdir -p "$TMP/freertos_stub/include"
echo 'add_library(freertos_kernel INTERFACE)' > "$TMP/freertos_stub/CMakeLists.txt"

cmake -S "$TMP/open62541" -B "$TMP/build" -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DUA_ARCHITECTURE=freertos-lwip -DUA_LWIP_EXTERNAL=ON \
    -DFREERTOS_KERNEL_PATH="$TMP/freertos_stub" -DFREERTOS_PORT_PATH="$TMP/freertos_stub" \
    -DUA_ENABLE_AMALGAMATION=ON -DUA_NAMESPACE_ZERO=REDUCED \
    -DUA_MULTITHREADING=0 -DUA_LOGLEVEL=300 \
    -DUA_ENABLE_PUBSUB=OFF -DUA_ENABLE_HISTORIZING=OFF -DUA_ENABLE_DISCOVERY=OFF \
    -DUA_ENABLE_XML_ENCODING=OFF -DUA_ENABLE_JSON_ENCODING=OFF -DUA_ENABLE_DIAGNOSTICS=OFF \
    -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=OFF \
    -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_NODEMANAGEMENT=OFF -DUA_ENABLE_ENCRYPTION=OFF \
    -DUA_BUILD_EXAMPLES=OFF -DUA_BUILD_UNIT_TESTS=OFF > /dev/null

cmake --build "$TMP/build" --target open62541-amalgamation > /dev/null

cp "$TMP/build/open62541.c" "$DESTINO/open62541.c"
cp "$TMP/build/open62541.h" "$DESTINO/include/open62541.h"
cp "$TMP/open62541/LICENSE" "$DESTINO/LICENSE"
echo "open62541 $VERSAO gerado em $DESTINO"
