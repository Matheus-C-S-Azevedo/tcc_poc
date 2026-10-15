# Prova de viabilidade: open62541 no ESP32-S3

Servidor OPC UA mínimo (open62541 v1.5.9, namespace 0 `REDUCED`, sem criptografia) com seis variáveis de diagnóstico atualizadas a cada 1 s, rodando no ESP32-S3-WROOM-1 N16R8 a 240 MHz, conectado por Wi-Fi.

## Cenários

Cada cenário muda apenas as opções do seu arquivo em [`cenarios/`](cenarios/):

| Cenário | Memória do open62541 | Economia de energia do Wi-Fi |
|---|---|---|
| A | SRAM interna (padrão do `malloc`) | ligada (modem sleep) |
| B | PSRAM (`CONFIG_OPEN62541_ALOCAR_PSRAM`) | ligada (modem sleep) |
| C | PSRAM | desligada (rádio sempre ativo) |

## Resultado 1: heap interno livre ao longo do boot

Medido pelo log serial (`tcc_poc: [etapa]` e `opcua: Heap após iniciar`), em 2026-10-14.

| Etapa | A | B | C |
|---|---|---|---|
| Boot | 272 KB | 271 KB | 271 KB |
| Após conectar no Wi-Fi | 220 KB | 220 KB | 220 KB |
| **Após iniciar o servidor OPC UA** | **84 KB** | **195 KB** | **195 KB** |
| PSRAM livre após o servidor | 8189 KB | 8079 KB | 8079 KB |

- No cenário A, o servidor consome ~136 KB da SRAM interna e a PSRAM fica intocada: o open62541 faz muitas alocações pequenas, e alocações abaixo de `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL` (16 KB) vão para a memória interna.
- No cenário B, ~110 KB passam para a PSRAM. Os ~25 KB internos restantes são quase todos a pilha da task OPC UA (24 KB).
- A economia de energia do Wi-Fi não altera o uso de memória (B = C).
- Com um cliente (UaExpert) conectado no cenário A, o maior bloco livre na SRAM interna caiu para 31 KB, o que mostra fragmentação.

## Resultado 2: memória após carga

Diagnóstico lido via OPC UA após cada rodada de 500 leituras (arquivos `resultados/info_*.txt`). Valores praticamente idênticos nas três rodadas de cada cenário.

| Após 500 leituras com cliente conectado | A | B | C |
|---|---|---|---|
| Heap interno livre | 88 KB | 204 KB | 202 KB |
| Mínimo atingido desde o boot | 81 KB | 195 KB | 191 KB |
| **Maior bloco contíguo livre** | **31 KB** | **120 KB** | **120 KB** |
| Folga mínima da pilha da task OPC UA (24 KB) | 19 KB | 19 KB | 19 KB |

## Resultado 3: latência de leitura

**Método:** 3 rodadas intercaladas (A, B, C, A, B, C, ...), cada uma com 500 leituras consecutivas de `TempoLigado`, feitas por um MacBook na mesma rede Wi-Fi (`project_tools/medir_cenarios.py`). Antes de cada rodada o firmware é regravado e o servidor estabiliza por 20 s. Timeout do cliente: 5 s. Firmware `34f9c07`. Data: 2026-10-15.

| Cenário | Leituras | Mínimo | **Mediana** | Média | p95 | p99 | Máximo | Desvio |
|---|---|---|---|---|---|---|---|---|
| A: SRAM | 1500 | 7,16 ms | **12,40 ms** | 16,45 ms | 33,18 ms | 67,78 ms | 295,02 ms | 14,07 ms |
| B: PSRAM | 1500 | 7,34 ms | **13,06 ms** | 16,61 ms | 34,54 ms | 51,53 ms | 146,55 ms | 10,43 ms |
| C: PSRAM, sem economia | 1500 | 7,61 ms | **12,63 ms** | 18,88 ms | 37,31 ms | 58,97 ms | 1253,70 ms | 52,18 ms |

Mediana e máximo de cada rodada:

| Cenário | Rodada 1 | Rodada 2 | Rodada 3 |
|---|---|---|---|
| A | 11,65 ms / 49,77 ms | 11,57 ms / 203,65 ms | 14,60 ms / 295,02 ms |
| B | 12,99 ms / 79,85 ms | 14,65 ms / 146,55 ms | 11,93 ms / 68,86 ms |
| C | 11,21 ms / 1253,70 ms | 12,40 ms / 1126,13 ms | 15,84 ms / 1096,17 ms |

## Conclusões

1. **A PSRAM não tem custo de latência mensurável neste cenário.** A diferença de mediana entre A e B (+0,66 ms) é menor que a variação entre rodadas do mesmo cenário (A: 11,6 a 14,6 ms). O tempo de processamento no servidor é pequeno diante do tempo de ida e volta pelo Wi-Fi.
2. **A PSRAM libera ~110 KB de SRAM interna** e reduz a fragmentação: o maior bloco contíguo livre passa de 31 KB para 120 KB. Com isso, a carga de um AAS real (árvore do cJSON, nós adicionais) deixa de competir com Wi-Fi e lwIP pela memória interna.
3. **Desligar a economia de energia do Wi-Fi não reduziu a mediana** e, neste teste, piorou a estabilidade: todas as rodadas do cenário C tiveram um travamento de ~1,1 a 1,25 s e houve 2 desconexões (`resultados/falhas.log`); A e B não tiveram nenhuma falha. A causa não foi determinada. Uma hipótese é perda de pacotes recuperada por retransmissão TCP (cerca de 1 s é um tempo de retransmissão típico); investigá-la exigiria captura de tráfego.
4. Leituras consecutivas mantêm o rádio acordado mesmo com a economia de energia ligada, então este teste não mede o efeito dela sobre requisições esporádicas. Um teste com leituras espaçadas (por exemplo, uma por segundo) mediria esse efeito.
5. A pilha da task OPC UA usa ~5 KB dos 24 KB reservados; dá para reduzir para algo como 12 KB depois de medir com o AAS real.

**Decisão:** manter `CONFIG_OPEN62541_ALOCAR_PSRAM=y` e a economia de energia do Wi-Fi ligada, que são os padrões do projeto.

**Limitações:** um único cliente, uma única rede, cliente também em Wi-Fi (a latência inclui dois enlaces sem fio), apenas o serviço de leitura (sem subscriptions sob carga) e sem criptografia.

## Como reproduzir

```bash
cd tcc_poc/firmware

# Compilar o cenário X (a, b ou c) em uma pasta de build própria
idf.py -B build_X -D SDKCONFIG=build_X/sdkconfig \
  -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;../avaliacao/prova_viabilidade/cenarios/<arquivo>.defaults" build

# Gravar e acompanhar o boot
idf.py -B build_X -p <porta> flash monitor

# Medição automática de todos os cenários (ferramenta em project_tools/):
# grava cada build_X, aguarda o servidor e mede latência e memória
python medir_cenarios.py --rodadas 3 -n 500
```

Os dados brutos ficam em [`resultados/`](resultados/): um CSV por rodada com o tempo de cada leitura, o diagnóstico de memória (`info_*.txt`), o consolidado (`resumo.csv`) e as falhas (`falhas.log`).

As credenciais de Wi-Fi ficam em `firmware/sdkconfig.local` (fora do git) e são aplicadas automaticamente em todos os cenários.
