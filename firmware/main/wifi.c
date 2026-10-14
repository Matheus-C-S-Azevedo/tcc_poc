#include "wifi.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#define BIT_CONECTADO BIT0

static const char *TAG = "wifi";

static EventGroupHandle_t s_eventos;
static esp_ip4_addr_t s_ip;

static void tratar_evento(void *arg, esp_event_base_t base, int32_t id, void *dados)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *evento = dados;
        xEventGroupClearBits(s_eventos, BIT_CONECTADO);
        ESP_LOGW(TAG, "Desconectado (motivo %d), tentando de novo...", evento->reason);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evento = dados;
        s_ip = evento->ip_info.ip;
        ESP_LOGI(TAG, "Conectado, IP " IPSTR, IP2STR(&s_ip));
        xEventGroupSetBits(s_eventos, BIT_CONECTADO);
    }
}

esp_err_t wifi_conectar(uint32_t timeout_ms, esp_ip4_addr_t *ip)
{
    if (strlen(CONFIG_TCC_WIFI_SSID) == 0) {
        ESP_LOGE(TAG, "SSID não configurado (defina CONFIG_TCC_WIFI_SSID em sdkconfig.local)");
        return ESP_ERR_INVALID_ARG;
    }

    s_eventos = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, tratar_evento, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, tratar_evento, NULL));

    wifi_config_t config = {
        .sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
    };
    strlcpy((char *)config.sta.ssid, CONFIG_TCC_WIFI_SSID, sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, CONFIG_TCC_WIFI_PASSWORD, sizeof(config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Conectando em \"%s\"...", CONFIG_TCC_WIFI_SSID);
    EventBits_t bits = xEventGroupWaitBits(s_eventos, BIT_CONECTADO, pdFALSE, pdTRUE,
                                           pdMS_TO_TICKS(timeout_ms));
    if (!(bits & BIT_CONECTADO)) {
        return ESP_ERR_TIMEOUT;
    }

    *ip = s_ip;
    return ESP_OK;
}
