#pragma once

#include <stdbool.h>
#include <sys/types.h>

#include <esp_err.h>
#include <rom/lldesc.h>

#define ESP32_DMA_MAX_SIZE 4092

#define ESP32_DMA_ALLIGNMENT 4
#define ESP32_DMA_ALIGN(val) (((val) + (ESP32_DMA_ALLIGNMENT - 1)) & ~((typeof(val))ESP32_DMA_ALLIGNMENT - 1))

#define ESP32_DMA_OWNER_CPU 0
#define ESP32_DMA_OWNER_DMA 1

typedef struct {
  volatile lldesc_t* descriptors;
  size_t num_descriptors;
  void* data;
} esp_dma_chain_t;

esp_err_t esp_dma_chain_alloc(esp_dma_chain_t* chain, size_t len, bool cyclic);
