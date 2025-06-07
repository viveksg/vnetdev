#ifndef DMA_CNTRL_
#define DMA_CNTRL_
#include "common.h"
#include <linux/types.h>
typedef struct{
    uint64_t buffer_address;
    uint16_t length;
    uint16_t packet_cheksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
}dma_descriptor;


typedef struct{
    uint16_t packet_cheksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
}dma_descriptor_fields;


void init_dma(uint32_t *registers);
void init_desc(dma_descriptor dma_desc, int index, int desc_type);
int dma_rx_op(uint8_t * buff);
int dma_tx_op(uint8_t *buff);

#endif