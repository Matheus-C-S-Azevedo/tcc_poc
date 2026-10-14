#include <inttypes.h>
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "aas_core.h"

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

void app_main(void)
{
    ESP_LOGI(TAG, "esp_aas_core v%s", aas_core_version());
    log_system_info();
}
