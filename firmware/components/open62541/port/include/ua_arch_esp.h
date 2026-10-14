/*
 * Definições de plataforma do open62541 para o ESP-IDF.
 *
 * Incluído no início de open62541.h via UA_ARCH_HEADER (definido no
 * CMakeLists.txt deste componente), antes dos valores padrão da biblioteca.
 */
#pragma once

#include <stddef.h>
#include "sdkconfig.h"

#if CONFIG_OPEN62541_ALOCAR_PSRAM
/* Redireciona toda a memória dinâmica do open62541 para a PSRAM,
 * preservando a SRAM interna para Wi-Fi, lwIP e pilhas das tasks. */
void *ua_esp_malloc(size_t size);
void *ua_esp_calloc(size_t num, size_t size);
void *ua_esp_realloc(void *ptr, size_t size);
void ua_esp_free(void *ptr);

#define UA_malloc  ua_esp_malloc
#define UA_calloc  ua_esp_calloc
#define UA_realloc ua_esp_realloc
#define UA_free    ua_esp_free
#endif
