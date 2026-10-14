#include "esp_log.h"
#include "aas_core.h"

static const char *TAG = "tcc_poc";

void app_main(void)
{
    ESP_LOGI(TAG, "esp_aas_core v%s", aas_core_version());
}
