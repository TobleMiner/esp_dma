#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <esp_heap_caps.h>

#include <esp_dma.h>

static inline size_t calc_required_num_descriptors(size_t len) {
  return (len + ESP32_DMA_MAX_SIZE - 1) / ESP32_DMA_MAX_SIZE;
}

esp_err_t esp_dma_chain_alloc(esp_dma_chain_t* chain, size_t len, bool cyclic) {
  esp_err_t err = 0;
  if(len < 1) {
    err = ESP_ERR_INVALID_ARG;
    goto fail;
  }

  size_t num_descriptors = calc_required_num_descriptors(len);
  // Length of fully filled descriptors
  size_t descriptor_len = num_descriptors * sizeof(lldesc_t);
  // Length of data for first N-1 descriptors
  size_t data_len = (num_descriptors - 1) * ESP32_DMA_MAX_SIZE;
  // Length of last descriptors data
  size_t last_len = len - data_len;
  // Last data fragment might not end on word boundary
  data_len += ESP32_DMA_ALIGN(last_len);

  lldesc_t* descriptors = heap_caps_malloc(descriptor_len, MALLOC_CAP_DMA);
  if(!descriptors) {
    err = ESP_ERR_NO_MEM;
    goto fail;
  }
  memset(descriptors, 0, descriptor_len);

  void* data = heap_caps_malloc(data_len, MALLOC_CAP_DMA);
  if(!data) {
    err = ESP_ERR_NO_MEM;
    goto fail_descriptors;
  }
  memset(data, 0, data_len);

  // Fill first N-1 dma descriptors
  uint8_t* data_ptr = data;
  lldesc_t* limit = descriptors + num_descriptors;
  lldesc_t* cursor = descriptors;
  lldesc_t* next = descriptors + 1;
  while(next < limit) {
    cursor->qe.stqe_next = next;
    cursor->size = ESP32_DMA_MAX_SIZE;
    cursor->length = ESP32_DMA_MAX_SIZE;
    cursor->buf = data_ptr;
    cursor->owner = ESP32_DMA_OWNER_DMA;
    
    cursor = next;
    next++;
    data_ptr += ESP32_DMA_MAX_SIZE;
  }

  // Fill Nth dma descriptor
  if(cyclic) {
    cursor->qe.stqe_next = descriptors;
  } else {
    cursor->qe.stqe_next = NULL;
    cursor->eof = 1;
  }
  cursor->size = ESP32_DMA_ALIGN(last_len);
  cursor->length = last_len;
  cursor->buf = data_ptr;
  cursor->owner = ESP32_DMA_OWNER_DMA;

  chain->descriptors = descriptors;
  chain->num_descriptors = num_descriptors;
  chain->data = data;

  return 0;

fail_descriptors:
  free(descriptors);
fail:
  return err;
}
