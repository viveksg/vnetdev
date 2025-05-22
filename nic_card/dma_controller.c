#include <stdint.h>
#include "includes/dma_controller.h"
uint32_t dma_cycles = 0;
uint32_t dst_addr = 0;
uint32_t data_count = 0;
uint8_t *tx_buffer;
uint8_t *rx_buffer;
dma_descriptor *tx_desc;
dma_descriptor *rx_desc;
uint32_t *reg_base;

void init_dma(uint32_t *registers)
{
    reg_base = registers;
    tx_desc = (dma_descriptor *)((void *)((uint64_t)registers[REG_TX_TBAH] << 32 | registers[REG_TX_TBAL]));
    rx_desc = (dma_descriptor *)((void *)((uint64_t)registers[REG_RX_RDBAH] << 32 | registers[REG_RX_RDBAL]));
}

void init_desc(dma_descriptor dma_desc, int index, int desc_type)
{
    if (desc_type == DESC_TYPE_RX)
    {
    }
    else if (desc_type == DESC_TYPE_TX)
    {
    }
}

int dma_rx_op(uint8_t *buff)
{
    uint32_t tail = reg_base[REG_RX_TAIL];
    uint32_t head = reg_base[REG_RX_HEAD];
    if((tail + 1)%DEFAULT_DESC_SIZE == head)
    {   
        reg_base[REG_RX_PACKET_DROP]++;
        return QUEUE_FULL;
    }

}

int dma_tx_op(uint8_t *buff)
{

}

//start a separate kernel thread to handle interrupts
