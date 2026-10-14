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

## Resultado 2: latência de leitura

*Pendente.* Procedimento: para cada cenário, gravar o firmware, aguardar o servidor e executar 1000 leituras consecutivas de `TempoLigado` a partir de um computador na mesma rede.

## Como reproduzir

```bash
cd tcc_poc/firmware

# Compilar o cenário X (a, b ou c) em uma pasta de build própria
idf.py -B build_X -D SDKCONFIG=build_X/sdkconfig \
  -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;../avaliacao/prova_viabilidade/cenarios/<arquivo>.defaults" build

# Gravar e acompanhar o boot
idf.py -B build_X -p <porta> flash monitor

# Latência (ferramenta em project_tools/)
python opcua_cliente.py latencia -n 1000 --csv latencia_X.csv
```

As credenciais de Wi-Fi ficam em `firmware/sdkconfig.local` (fora do git) e são aplicadas automaticamente em todos os cenários.
