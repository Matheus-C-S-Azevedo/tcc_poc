#include "ua_arch_esp.h"
#include "esp_heap_caps.h"

/* Prefere a PSRAM; se ela estiver cheia, usa a memória padrão (SRAM interna) */
#define CAPS_PREFERIDAS (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define CAPS_RESERVA    MALLOC_CAP_DEFAULT

void *ua_esp_malloc(size_t size)
{
    return heap_caps_malloc_prefer(size, 2, CAPS_PREFERIDAS, CAPS_RESERVA);
}

void *ua_esp_calloc(size_t num, size_t size)
{
    return heap_caps_calloc_prefer(num, size, 2, CAPS_PREFERIDAS, CAPS_RESERVA);
}

void *ua_esp_realloc(void *ptr, size_t size)
{
    return heap_caps_realloc_prefer(ptr, size, 2, CAPS_PREFERIDAS, CAPS_RESERVA);
}

void ua_esp_free(void *ptr)
{
    heap_caps_free(ptr);
}
