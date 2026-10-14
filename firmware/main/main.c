#include <inttypes.h>
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "nvs_flash.h"
#include "aas_core.h"
#include "opcua_server.h"
#include "wifi.h"

#define TIMEOUT_WIFI_MS 30000

static const char *TAG = "tcc_poc";

// Confirma no boot que a configuração da placa (N16R8) foi aplicada
static void log_system_info(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "%s rev v%d.%d, %d núcleos a %" PRIu32 " MHz",
             CONFIG_IDF_TARGET, chip.revision / 100, chip.revision % 100,
             chip.cores, esp_rom_get_cpu_ticks_per_us());
    ESP_LOGI(TAG, "Flash: %" PRIu32 " MB", flash_size / (1024 * 1024));
    ESP_LOGI(TAG, "PSRAM: %zu MB", esp_psram_get_size() / (1024 * 1024));
    ESP_LOGI(TAG, "Heap interno: %zu KB livres de %zu KB",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024,
             heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024);
    ESP_LOGI(TAG, "Heap PSRAM:   %zu KB livres de %zu KB",
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024,
             heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024);
}

// Registra o heap em cada etapa do boot para medir o custo de cada camada
static void log_heap(const char *etapa)
{
    ESP_LOGI(TAG, "[%s] heap interno %zu KB (maior bloco %zu KB), PSRAM %zu KB livres",
             etapa,
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024,
             heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024,
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
}

static void iniciar_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    ESP_LOGI(TAG, "esp_aas_core v%s", aas_core_version());
    log_system_info();
    log_heap("boot");

    iniciar_nvs();

    esp_ip4_addr_t ip;
    esp_err_t err = wifi_conectar(TIMEOUT_WIFI_MS, &ip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sem Wi-Fi (%s); servidor OPC UA não iniciado", esp_err_to_name(err));
        return;
    }
    log_heap("wifi");

    opcua_server_iniciar(&ip);
}
