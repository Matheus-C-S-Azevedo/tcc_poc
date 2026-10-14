#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_netif_ip_addr.h"

/**
 * @brief Conecta na rede definida em CONFIG_TCC_WIFI_SSID e aguarda receber IP.
 *
 * Após a primeira conexão, reconecta sozinho se a rede cair.
 *
 * @param timeout_ms Tempo máximo de espera pelo IP.
 * @param ip         Recebe o IP atribuído à placa.
 */
esp_err_t wifi_conectar(uint32_t timeout_ms, esp_ip4_addr_t *ip);
