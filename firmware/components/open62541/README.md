# open62541 (amalgamação para ESP-IDF)

Pilha OPC UA [open62541](https://github.com/open62541/open62541) **v1.5.9**, empacotada como componente ESP-IDF a partir da amalgamação oficial (`open62541.c` + `include/open62541.h`).

- **Licença:** MPL-2.0 (ver [LICENSE](LICENSE)). Os arquivos deste diretório não devem ser modificados; adaptações para o ESP32 ficam em arquivos próprios do projeto.
- **Não editar à mão:** para atualizar ou mudar opções, edite e rode `./gerar_amalgamacao.sh`.

## Configuração gerada

| Opção | Valor | Motivo |
|---|---|---|
| `UA_ARCHITECTURE` | `freertos-lwip` | FreeRTOS + lwIP do ESP-IDF |
| `UA_NAMESPACE_ZERO` | `REDUCED` | Namespace 0 reduzido, economiza memória |
| `UA_MULTITHREADING` | `0` | Não há travas para FreeRTOS; a API só é chamada pela task do OPC UA |
| `UA_LOGLEVEL` | `300` (info) | |
| Subscriptions, métodos | ligados | Monitoramento de valores pelo cliente |
| PubSub, histórico, discovery, XML/JSON, diagnósticos, criptografia, node management | desligados | Fora do escopo da prova de viabilidade |

## Adaptações para o ESP-IDF (arquivos próprios, fora da MPL)

| Arquivo | Função |
|---|---|
| `port/include/ua_arch_esp.h` | Injetado via `UA_ARCH_HEADER` antes dos padrões do open62541. Com `CONFIG_OPEN62541_ALOCAR_PSRAM` (menuconfig → open62541, ligado por padrão), redireciona `UA_malloc`/`UA_calloc`/`UA_realloc`/`UA_free` para a PSRAM. |
| `port/ua_arch_esp.c` | Alocador: prefere a PSRAM e cai para a SRAM interna se ela estiver cheia. |
| `port/freertos_compat/` | `FreeRTOS.h` e `task.h`, que redirecionam para `freertos/...` (no ESP-IDF v6 o diretório original é privado). |

## Limitação conhecida

O relógio do porte FreeRTOS conta o tempo desde o boot: os timestamps OPC UA aparecem como 1970 + tempo ligado, até a sincronização de horário (SNTP) ser tratada.
