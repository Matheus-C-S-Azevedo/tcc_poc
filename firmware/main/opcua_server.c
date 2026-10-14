#include "opcua_server.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "open62541.h"

#define PILHA_TASK      (24 * 1024)
#define PRIORIDADE_TASK 5
#define NUCLEO_TASK     0
#define BUFFER_TCP      (16 * 1024)

static const char *TAG = "opcua";

static esp_ip4_addr_t s_ip;
static UA_UInt16 s_ns;

static void adicionar_variavel(UA_Server *server, const UA_NodeId *pai,
                               char *nome, char *descricao)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 zero = 0;
    UA_Variant_setScalar(&attr.value, &zero, &UA_TYPES[UA_TYPES_UINT32]);
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.displayName = UA_LOCALIZEDTEXT("pt-BR", nome);
    attr.description = UA_LOCALIZEDTEXT("pt-BR", descricao);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_Server_addVariableNode(server, UA_NODEID_STRING(s_ns, nome), *pai,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(s_ns, nome),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);
}

static void criar_nos(UA_Server *server)
{
    s_ns = UA_Server_addNamespace(server, "urn:tcc:poc");

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("pt-BR", "Diagnostico");
    UA_NodeId pasta = UA_NODEID_STRING(s_ns, "Diagnostico");
    UA_Server_addObjectNode(server, pasta, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(s_ns, "Diagnostico"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), attr, NULL, NULL);

    adicionar_variavel(server, &pasta, "HeapInternoLivre", "SRAM interna livre agora (bytes)");
    adicionar_variavel(server, &pasta, "HeapInternoMinimo", "Menor SRAM interna livre desde o boot (bytes)");
    adicionar_variavel(server, &pasta, "HeapInternoMaiorBloco", "Maior bloco contíguo livre na SRAM interna (bytes)");
    adicionar_variavel(server, &pasta, "HeapPsramLivre", "PSRAM livre agora (bytes)");
    adicionar_variavel(server, &pasta, "PilhaOpcuaLivre", "Menor folga de pilha da task OPC UA (bytes)");
    adicionar_variavel(server, &pasta, "TempoLigado", "Tempo desde o boot (s)");
}

static void escrever_u32(UA_Server *server, char *nome, UA_UInt32 valor)
{
    UA_Variant v;
    UA_Variant_setScalar(&v, &valor, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, UA_NODEID_STRING(s_ns, nome), v);
}

static void atualizar_diagnostico(UA_Server *server, void *dados)
{
    escrever_u32(server, "HeapInternoLivre", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    escrever_u32(server, "HeapInternoMinimo", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
    escrever_u32(server, "HeapInternoMaiorBloco", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    escrever_u32(server, "HeapPsramLivre", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    escrever_u32(server, "PilhaOpcuaLivre", uxTaskGetStackHighWaterMark(NULL));
    escrever_u32(server, "TempoLigado", esp_timer_get_time() / 1000000);
}

static void configurar(UA_ServerConfig *config)
{
    UA_ServerConfig_setMinimalCustomBuffer(config, CONFIG_TCC_OPCUA_PORT, NULL,
                                           BUFFER_TCP, BUFFER_TCP);
    config->maxSecureChannels = 4;
    config->maxSessions = 4;

    // Identificação do servidor (aparece em Server > ServerStatus > BuildInfo)
    UA_BuildInfo_clear(&config->buildInfo);
    config->buildInfo.productName = UA_STRING_ALLOC("tcc_poc");
    config->buildInfo.productUri = UA_STRING_ALLOC("urn:tcc:poc");
    config->buildInfo.manufacturerName = UA_STRING_ALLOC("Matheus C. S. Azevedo");
    config->buildInfo.softwareVersion = UA_STRING_ALLOC(esp_app_get_description()->version);

    UA_LocalizedText_clear(&config->applicationDescription.applicationName);
    config->applicationDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("pt-BR", "TCC PoC - AAS embarcado");

    // Anuncia o endpoint pelo IP: o hostname da placa não é resolvido pelos clientes
    char url[48];
    snprintf(url, sizeof(url), "opc.tcp://" IPSTR ":%d", IP2STR(&s_ip), CONFIG_TCC_OPCUA_PORT);
    UA_Array_delete(config->serverUrls, config->serverUrlsSize, &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls = UA_String_new();
    *config->serverUrls = UA_STRING_ALLOC(url);
    config->serverUrlsSize = 1;
}

static void opcua_task(void *arg)
{
    UA_ServerConfig config;
    memset(&config, 0, sizeof(config));
    configurar(&config);

    UA_Server *server = UA_Server_newWithConfig(&config);
    if (server == NULL) {
        ESP_LOGE(TAG, "Falha ao criar o servidor");
        vTaskDelete(NULL);
        return;
    }

    criar_nos(server);
    atualizar_diagnostico(server, NULL);
    UA_Server_addRepeatedCallback(server, atualizar_diagnostico, NULL, 1000, NULL);

    UA_StatusCode rc = UA_Server_run_startup(server);
    if (rc != UA_STATUSCODE_GOOD) {
        ESP_LOGE(TAG, "Falha ao iniciar: %s", UA_StatusCode_name(rc));
        UA_Server_delete(server);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Servidor em opc.tcp://" IPSTR ":%d", IP2STR(&s_ip), CONFIG_TCC_OPCUA_PORT);
    ESP_LOGI(TAG, "Heap após iniciar: interno %zu KB, PSRAM %zu KB livres",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024,
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);

    // Bloqueia em select() até o próximo evento de rede ou callback agendado
    while (true) {
        UA_Server_run_iterate(server, true);
    }
}

void opcua_server_iniciar(const esp_ip4_addr_t *ip)
{
    s_ip = *ip;
    xTaskCreatePinnedToCore(opcua_task, "opcua", PILHA_TASK, NULL,
                            PRIORIDADE_TASK, NULL, NUCLEO_TASK);
}
