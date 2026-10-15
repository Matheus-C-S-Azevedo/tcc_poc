# tcc_poc

Prova de conceito do TCC: gêmeo digital (AAS Tipo 2) de uma carga elétrica real, instrumentado com ESP32-S3 e o componente [esp_aas_core](https://github.com/Matheus-C-S-Azevedo/esp_aas_core).

## Estrutura

```
tcc_poc/
├── hardware/    # esquemático, PCB, BOM e gerbers (KiCad)
├── firmware/    # aplicação ESP-IDF que usa o esp_aas_core
└── aas/         # AASX e JSON do ativo monitorado
```

## Hardware

- ESP32-S3-WROOM-1 N16R8 (16 MB flash, 8 MB PSRAM)
- Sensor de corrente ACS712
- Sensor de tensão ZMPT101B
- Módulo de cartão Micro SD (SPI)
- Fonte Hi-Link HLK-5M05

## Compilando o firmware

O firmware usa a cópia local do `esp_aas_core` durante o desenvolvimento, então os dois repositórios devem ser clonados lado a lado:

```
TCC Project/
├── esp_aas_core/
└── tcc_poc/
```

```bash
cd firmware
idf.py build flash monitor
```
