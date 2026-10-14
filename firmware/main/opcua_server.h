#pragma once

#include "esp_netif_ip_addr.h"

/**
 * @brief Cria a task do servidor OPC UA (núcleo 0) anunciando o endpoint no IP dado.
 *
 * Toda chamada à API do open62541 acontece dentro dessa task: a biblioteca foi
 * gerada sem multithreading (UA_MULTITHREADING = 0).
 */
void opcua_server_iniciar(const esp_ip4_addr_t *ip);
