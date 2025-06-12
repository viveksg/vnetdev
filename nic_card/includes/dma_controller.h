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



#endif