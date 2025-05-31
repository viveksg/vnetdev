#ifndef __NIC_CARD__
#define _NIC_CARD__

#define MTU_VAL 1500
#define REG_BITS 32
#define BUFFER_NUM 64
#define BUFFER_SIZE MTU_VAL*BUFFER_NUM //size of ring buffer
struct dma_controller{
    uint32_t data_count_register; //number of words to be transfered
    uint32_t data_register; // single word
    uint32_t address_register; //address of word
    uint32_t csr_dma_request; 
    uint32_t dma_ack;
    uint32_t read_enable;
    uint32_t write_enable;
    uint32_t intr; //interrupt register each bit representing a type of interrupt

};

struct nic_card{
    uint8_t mac_addr[6];
    struct dma_controller dma_cntrl;
    uint8_t rx_link_buffer[MTU_VAL];
    uint8_t tx_link_buffer[MTU_VAL];
    uint8_t * rx_ring;
    uint8_t *tx_ring;
};


#endif