/*
 * Compatibilidade: o porte FreeRTOS do open62541 usa #include <task.h>,
 * mas no ESP-IDF v6 esse diretório é privado. Redireciona para o caminho público.
 */
#pragma once
#include "freertos/task.h"
