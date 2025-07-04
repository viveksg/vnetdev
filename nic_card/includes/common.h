#ifndef __COMMON__
#define __COMMON__
#define DESC_TYPE_TX 1001
#define DESC_TYPE_RX 1002
#define QUEUE_FULL 1003
#define SUCCESS_OP 1004
#define DESC_COUNT 128
#define DESC_SIZE 16
#define MTU 1500
#define DEFAULT_TIMER 1000
#define QUEUE_OP_ENQUEUE 1001
#define QUEUE_OP_DEQUEUE 1002
#define QUEUE_FULL 1003
#define QUEUE_EMPTY 1004
#define QUEUE_OP_SUCCESS 1005
#define COPY_FROM_QUEUE 1
#define RES_NAME_REG_BASE "reg_base"
#define PLAT_DEVICE_NAME "net_plat_dev"

#define DEVICE_MEM 65535
#define SINGLE_BUFFER_MEM MTU
#define REG_MEM_SIZE 4096
#define TX_DESC_MEM (DESC_COUNT * DESC_SIZE)
#define RX_DESC_MEM (DESC_COUNT * DESC_SIZE)
#define TX_BUFFER_MEM (SINGLE_BUFFER_MEM * DESC_COUNT)
#define RX_BUFFER_MEM (SINGLE_BUFFER_MEM * DESC_COUNT)
#define TOTAL_MEM (TX_DESC_MEM + RX_DESC_MEM + TX_BUFFER_MEM + RX_BUFFER_MEM)
#define REG_OFFSET 0
#define TX_DESC_MEM_OFFSET 0//(REG_MEM_SIZE)
#define RX_DESC_MEM_OFFSET (TX_DESC_MEM_OFFSET + TX_DESC_MEM)
#define TX_BUFFER_OFFSET (RX_DESC_MEM_OFFSET + RX_DESC_MEM)
#define RX_BUFFER_OFFSET (TX_BUFFER_OFFSET + TX_BUFFER_MEM)
#define ETH_ALEN 6
#define DRIVER_WORKQUEUE "driver_workqueue"
#define CARD_WORKQUEUE "card_workqueue"
#define DEFAULT_NETWORK_DEV_NAME "vnic%d"
#define DEVICE_STATUS_DISABLED 0
#define DEVICE_STATUS_ENABLED 0xFFFF

#define RX_STATUS_ENABLE_EOP 0x02
#define RX_STATUS_ENABLE_DD 0x01
#define RX_EOP_MASK 0xFD
#define RX_DD_MASK 0xFE


#define TX_STATUS_ENABLE_EOP 0x02
#define TX_STATUS_ENABLE_DD 0x01
#define TX_EOP_MASK 0xFD
#define TX_DD_MASK 0xFE

#define PACKET_NOT_READY 1001
#define PACKET_READY 1002

#define MAC_ADDR_AL 0xAABBCCDD
#define MAC_ADD_AH 0x00001122
#endif