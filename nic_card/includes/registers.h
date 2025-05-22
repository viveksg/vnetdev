#define NUM_REGISTERS 46

//general
#define REG_DEVICE_CTRL 0
#define REG_DEVICE_STATUS 1
#define REG_DEVICE_EXTENDED_DEVICE_CONTROL 2
#define REG_DEVICE_MDI 3
#define REG_FLOW_CONTROL_ADDR_L 4
#define REG_FLOW_CONTROL_ADD_H 5
#define REG_FLOW_CONTROL_TYPE 6
#define REG_VLAN_ETHER_TYPE 7
#define REG_FLOW_CONTROL_TRANSMIT_TIMER_VAL 8

//DMA
#define REG_PACKET_BUFFER_ALLOCATION 9
#define REG_TX_DMAC 10 //TX DMA CONTROL
#define REG_TX_DCTL 11 // Transmist Descriptor Control
#define REG_TSPMT 12 // TCP segmentation pad and threshold
#define REG_RX_DCTL 13 // Transmit Descriptor Control
#define REG_RX_RXCSUM 14 // Receive checksum

//Interrupt
#define REG_ICR 15  // Interrupt cause read
#define REG_ICS 16 // Interrupt cause set
#define REG_IMS 17 // Interrupt mask set/read
#define REG_IMC 18 // Interrupt mask clear

//Receive

#define REG_RX_RCTL 19 //RECEIVE CONTROL
#define REG_RX_FCRTL 20 //FLOW CONTROL RECEIVE CONTROL THRESHOLD LOW
#define REG_RX_FCRTH 21 //FLOW CONTROL RECEIVE CONTROL THRESHOLD HIGH
#define REG_RX_RDBAL 22 //Receive descriptor base low
#define REG_RX_RDBAH 23 //Receive descriptor base high
#define REG_RX_RDLEN 24 //Receive descriptor length
#define REG_RX_HEAD 25 //Receive descriptor head
#define REG_RX_TAIL 26 //Receive descriptor tail
#define REG_RX_RDTR 27 //Receive delay timer

//Transmit
#define REG_TX_TCTL 28 // Transmit Control
#define REG_TX_TIPG 29 // Transmit TIPG
#define REG_TX_AIFS 30 // Adaptive IFS throttle -AIT
#define REG_TX_TBAL 31 //Transmit Descriptor Base low
#define REG_TX_TBAH 32 //Trasnmit Descriptor Base High
#define REG_TX_TDLEN 33 //Trasnmit Descriptor Length
#define REG_TX_TDH 34 //Transmit Descriptor Head
#define REG_TX_TDT 35 //Transmit Descriptor Tail
#define REG_TX_TIDTV 36 // Transmit Interrupt Delay Timer

//Address
#define REG_MTA 37// multicast table array
#define REG_RAL 38 // Receive address low
#define REG_RAH 39 // Receive address high

// Stats
#define REG_TX_GOOD_PACKETS 40
#define REG_RX_GOOD_PACKETS 41
#define REG_TX_GOOD_OCTETS 42
#define REG_RX_GOOD_OCTESTS 43
#define REG_RX_PACKET_DROP 44
#define REG_TX_PACKET_DROP 45

uint32_t get_bytes(uint32_t shift, uint32_t length, uint32_t val);