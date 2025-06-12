
#include "registers.h"
#include "common.h"
#include "dma_controller.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/etherdevice.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt, __func__
struct net_device *net_pdev;
static struct workqueue_struct *nic_wq;
static struct delayed_work nic_poll_work;

struct pndev_priv
{
    void __iomem *reg_base;
    int tx_irq;
    int rx_irq;
    struct net_device *ndev;
};

uint8_t temp_rx_buffer[DEVICE_MEM];

dma_descriptor *tx_desc;
dma_descriptor *rx_desc;
uint32_t *reg_base;
uint8_t *tx_packet_buff;
uint8_t *rx_packet_buff;
uint8_t *mem;

int queue_op(uint32_t op, dma_descriptor *qbase, uint32_t qhead_ind, uint32_t qtail_ind, uint32_t qsize, uint8_t *buff, uint16_t buff_len, int op_direction, dma_descriptor_fields fields)
{
    uint16_t qtail = reg_base[qtail_ind];
    uint16_t qhead = reg_base[qhead_ind];
    pr_info("queue head %d, queue_tail %d\n", qhead, qtail);
    pr_info("queue base %x", qbase);
    if (op == QUEUE_OP_ENQUEUE)
    {
        if ((qtail + 1) % qsize == qhead)
        {
            pr_info("Queue full, dropping packet\n");
            return QUEUE_FULL;
        }
        pr_info("Attempting to insert packet in queue\n");
        uint8_t *qbuffer = phys_to_virt(qbase[qtail].buffer_address);
        pr_info("queue buffer %x", qbuffer);
        qbase[qtail].status |= fields.status;
        if (op_direction == COPY_FROM_QUEUE)
            memcpy(buff, qbuffer, qbase[qtail].length);
        else
        {
            memcpy(qbuffer, buff, buff_len);
            qbase[qtail].length = buff_len;
            qtail = (qtail + 1) % qsize;
        }
    }
    else
    {
        if (qhead == qtail)
            return QUEUE_EMPTY;
        qhead = (qhead + 1) % qsize;
    }
    reg_base[qtail_ind] = qtail;
    reg_base[qhead_ind] = qhead;
    return QUEUE_OP_SUCCESS;
}

void reset_desc_registers(void)
{
    uint64_t tx_base = virt_to_phys(tx_desc);
    uint64_t rx_base = virt_to_phys(rx_desc);

    reg_base[REG_RX_RDBAL] = (rx_base & 0xFFFFFFFF);
    reg_base[REG_RX_RDBAH] = rx_base >> 32;
    reg_base[REG_RX_HEAD] = 0;
    reg_base[REG_RX_TAIL] = 0;
    reg_base[REG_RX_RDLEN] = DESC_COUNT;

    reg_base[REG_TX_TBAL] = (tx_base & 0xFFFFFFFF);
    reg_base[REG_TX_TBAH] = tx_base >> 32;
    reg_base[REG_TX_TDH] = 0;
    reg_base[REG_TX_TDT] = 0;
    reg_base[REG_TX_TDLEN] = DESC_COUNT;
    printk(KERN_INFO "tx base : %x, reg_al: %x, reg_ax: %x", tx_base, reg_base[REG_TX_TBAL], reg_base[REG_TX_TBAH]);
    printk(KERN_INFO "rx base : %x, reg_al: %x, reg_ax: %x", rx_base, reg_base[REG_RX_RDBAL], reg_base[REG_RX_RDBAH]);

    // enable device
    reg_base[REG_DEVICE_STATUS] = DEVICE_STATUS_ENABLED;
}

void init_descriptors(dma_descriptor *base, uint32_t buffer_size, uint8_t *buffer_base, uint32_t num_descriptors)
{
    uint8_t *curr_buffer = buffer_base;
    for (int i = 0; i < num_descriptors; i++)
    {
        base[i].buffer_address = virt_to_phys(curr_buffer);
        curr_buffer += buffer_size;
    }
}

void print_descriptors(dma_descriptor *base, int count)
{
    for (int i = 0; i < count; i++)
    {
        pr_info("Descriptor base virtual= %x, physical = %x,\n", phys_to_virt(base[i].buffer_address), base[i].buffer_address);
    }
}

void init_alloc(void)
{
    mem = kmalloc(TOTAL_MEM, GFP_KERNEL);
    memset(mem, 0, TOTAL_MEM);
    tx_desc = (dma_descriptor *)((char *)mem + TX_DESC_MEM_OFFSET);
    rx_desc = (dma_descriptor *)((char *)mem + RX_DESC_MEM_OFFSET);
    tx_packet_buff = (char *)((char *)mem + TX_BUFFER_OFFSET);
    rx_packet_buff = (char *)((char *)mem + RX_BUFFER_OFFSET);

    pr_info("Allocated %ld bytes at address %p", TOTAL_MEM, (char *)mem);
    pr_info("Address details:\n");
    pr_info("Address reg_base: %x\n", (uint64_t)reg_base);
    pr_info("Address tx_desc: %x\n", ((uint64_t)tx_desc));
    pr_info("Address tx_desc:- %x\n", (((uint64_t)mem + 4096)));
    pr_info("Address rx_desc: %x\n", (uint64_t)rx_desc);
    pr_info("Address tx_packet_buff: %x\n", (uint64_t)tx_packet_buff);
    pr_info("Address rx_packet_buff: %x\n", (uint64_t)rx_packet_buff);
}

void reset_device(void)
{
    init_alloc();
    init_descriptors(tx_desc, MTU, tx_packet_buff, DESC_COUNT);
    init_descriptors(rx_desc, MTU, tx_packet_buff, DESC_COUNT);
    print_descriptors(tx_desc, DESC_COUNT);
    print_descriptors(rx_desc, DESC_COUNT);
    reset_desc_registers();
}

void send_packet_to_protocol_stack(uint8_t *packet_data, uint32_t len)
{
    pr_info("Attempting to send packet to protocol stack");
    struct sk_buff *skbuff;
    struct ethhdr *header;
    skbuff = netdev_alloc_skb(net_pdev, len);
    if (!skbuff)
    {
        pr_err("cannot allocate sk_buff");
        return;
    }
    skb_reserve(skbuff, 2);
    memcpy(skb_put(skbuff, len), packet_data, len);
    header = (struct ethhdr *)skb_push(skbuff, sizeof(struct ethhdr));
    header->h_proto = htons(ETH_P_IP);
    skbuff->dev = net_pdev;
    skbuff->protocol = htons(ETH_P_IP);
    netif_rx(skbuff);
    pr_info("Packet transferred");
    reg_base[REG_RX_GOOD_PACKETS]++;
}

static void process_receive_buffers(void)
{
    if (reg_base[REG_RX_TAIL] != reg_base[REG_RX_HEAD])
    {
        int qlen = DESC_COUNT;
        int tail = reg_base[REG_RX_TAIL];
        int head = reg_base[REG_RX_HEAD];
        dma_descriptor *base = (dma_descriptor *)phys_to_virt(((uint64_t)reg_base[REG_RX_RDBAH] << 32) | reg_base[REG_RX_RDBAL]);
        uint32_t buffer_idx = 0;
        pr_info("qhead = %x, qtail = %x", head, tail);
        pr_info("reg_ah %x, reg_al %x, computed base %x ", reg_base[REG_RX_RDBAH], reg_base[REG_RX_RDBAL], base);
        // printk(KERN_INFO," buffer_addr %x, phys addr %x",buffer_addr,base[i].buffer_address);
        for (int i = head; i != tail; i = (i + 1) % qlen)
        {

            uint8_t *buffer_addr = (uint8_t *)phys_to_virt(base[i].buffer_address);
            pr_info(" buffer_addr %x, phys addr %x", buffer_addr, base[i].buffer_address);
            pr_info("buffer_length = %x, status  = %d, status_check = %d,buffer_idx = %d", base[i].length, base[i].status, ((base[i].status & RX_STATUS_ENABLE_EOP) == RX_STATUS_ENABLE_EOP), buffer_idx);
            memcpy(&temp_rx_buffer[buffer_idx], buffer_addr, base[i].length);
            buffer_idx += base[i].length;
            if ((base[i].status & RX_STATUS_ENABLE_EOP) == RX_STATUS_ENABLE_EOP)
            {
                send_packet_to_protocol_stack(temp_rx_buffer, buffer_idx);
                buffer_idx = 0;
            }
        }
        reg_base[REG_RX_TAIL] = head;
    }
}

static void nic_polling_function(struct work_struct *work)
{
    process_receive_buffers();
    queue_delayed_work(nic_wq, &nic_poll_work, msecs_to_jiffies(DEFAULT_TIMER));
}

static int net_open(struct net_device *sndev)
{
    sndev->flags != IFF_UP;
    netif_start_queue(sndev);
    netif_carrier_on(sndev);
    nic_wq = create_singlethread_workqueue(DRIVER_WORKQUEUE);
    if (!nic_wq)
    {
        printk(KERN_ERR "cant create work queue in driver");
        return -ENOMEM;
    }
    INIT_DELAYED_WORK(&nic_poll_work, nic_polling_function);
    queue_delayed_work(nic_wq, &nic_poll_work, msecs_to_jiffies(DEFAULT_TIMER));
    printk(KERN_INFO "nic open completed");
    return 0;
}

static int net_close(struct net_device *sndev)
{
    netif_carrier_off(sndev);
    netif_stop_queue(sndev);

    if (nic_wq)
    {
        cancel_delayed_work_sync(&nic_poll_work);
        destroy_workqueue(nic_wq);
    }
    printk(KERN_INFO "nic driver closed");
    return 0;
}

/**
 * This function will put the skbuff data in the transmit DMA buffer
 * by enqueueing elements in tx_dma descripors
 */
static netdev_tx_t net_xmit(struct sk_buff *sbuff, struct net_device *sndev)
{
    int buff_len = sbuff->len;
    int dma_buff_len = SINGLE_BUFFER_MEM;
    uint32_t qhead = reg_base[REG_TX_TDH];
    uint32_t qtail = reg_base[REG_TX_TDT];
    uint32_t qsize = DESC_COUNT;
    dma_descriptor_fields fields;
    fields.status = 0;
    int queue_op_status = 0;
    if (buff_len > dma_buff_len)
    {
        int queue_ops = (buff_len/dma_buff_len + (buff_len % dma_buff_len > 0? 1 : 0));
        uint32_t transfer_len = 0;
        uint32_t offset = 0;
        for(int i = 0; i < queue_ops; i++, offset+=dma_buff_len)
        {
            if(i < queue_ops -1)
            {
                fields.status = 0;
                transfer_len = dma_buff_len;
            }
            else
            {
                fields.status |= TX_STATUS_ENABLE_EOP;
                transfer_len = buff_len - offset;
            }
           // for(int j = 0; j < sbuff->len;j++)
           // {
          //      pr_info("byte %d , data = %c",j,sbuff->data[j]);
           // }
            queue_op_status = queue_op(QUEUE_OP_ENQUEUE, tx_desc, REG_TX_TDH, REG_TX_TDT, DESC_COUNT, &(sbuff->data[offset]), transfer_len, 0, fields);
        }
    }
    else
    {
       // for (int j = 0; j < sbuff->len; j++)
       // {
       //     pr_info("byte %d , data = %c", j,sbuff->data[j]);
       // }
        fields.status |= TX_STATUS_ENABLE_EOP;
        queue_op_status = queue_op(QUEUE_OP_ENQUEUE, tx_desc, REG_TX_TDH, REG_TX_TDT, DESC_COUNT, sbuff->data, sbuff->len, 0, fields);
    }
    if (queue_op_status == QUEUE_OP_SUCCESS)
    {
        qtail = (qtail + 1) % qsize;
        reg_base[REG_TX_TDT] = qtail;
        pr_info("transmit packet added to the dma qhead = %d, qtail = %d", qhead, qtail);
    }
    return NETDEV_TX_OK;
}

static const struct net_device_ops net_ops = {
    .ndo_open = net_open,
    .ndo_stop = net_close,
    .ndo_start_xmit = net_xmit

};

static void net_dev_setup(struct net_device *ndev)
{
}

static int init_net_device(void)
{
    net_pdev = alloc_netdev(0, "nic%d", NET_NAME_UNKNOWN, net_dev_setup);
    if (!net_pdev)
    {
        printk(KERN_ERR "Network device allocation failed");
        return -ENOMEM;
    }
    if (register_netdev(net_pdev))
    {
        printk(KERN_ERR "Device registration failed");
        free_netdev(net_pdev);
        return -1;
    }
    printk(KERN_INFO "Device successfully registered");
    return 0;
}

static void set_mac_address(struct net_device *ndev)
{
    uint32_t mac_lo = reg_base[REG_RAL];
    uint32_t mac_hi = reg_base[REG_RAH];
    uint8_t mac_addr[6];
    mac_addr[0] = mac_lo & 0xFF;
    mac_addr[1] = (mac_lo >> 8) & 0xFF;
    mac_addr[2] = (mac_lo >> 16) & 0xFF;
    mac_addr[3] = (mac_lo >> 24) & 0xFF;
    mac_addr[4] = mac_hi & 0xFF;
    mac_addr[5] = (mac_hi >> 8) & 0xFF;
    //memcpy(ndev->dev_addr,mac_addr,ETH_ALEN);
    eth_hw_addr_set(ndev,mac_addr);
    ndev->flags |= (IFF_BROADCAST | IFF_MULTICAST);
}

static int net_dev_probe(struct platform_device *pdev)
{
    printk("probe function called\n");

    struct pndev_priv *priv;
    struct resource *res;
    net_pdev = alloc_etherdev(sizeof(struct pndev_priv));
    strncpy(net_pdev->name,DEFAULT_NETWORK_DEV_NAME,IFNAMSIZ);
    if (!net_pdev)
    {
        printk(KERN_ERR "allocation of nety device failed");
        return -ENOMEM;
    }
    priv = netdev_priv(net_pdev);
    platform_set_drvdata(pdev, net_pdev);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    priv->reg_base = (void __iomem *)phys_to_virt(res->start); // devm_ioremap_resource(&pdev->dev,res);
    printk(KERN_INFO "io base : %x", priv->reg_base);
    if (IS_ERR(priv->reg_base))
    {
        printk(KERN_ERR "Error in fetching reg_base from the resource");
        return PTR_ERR(priv->reg_base);
    }
    printk(KERN_INFO "io base : %x", priv->reg_base);
    reg_base = (uint32_t *)priv->reg_base;
    printk(KERN_INFO " reg_base %x", reg_base);
    reset_device();
    printk(KERN_INFO "registering device");
    net_pdev->netdev_ops = &net_ops;
    priv->ndev = net_pdev;
    //eth_hw_addr_random(net_pdev);
    set_mac_address(net_pdev);
    int err = register_netdev(net_pdev);
    printk(KERN_ERR "err_val %d", err);
    int x = 0;
    if (err != 0)
    {
        printk(KERN_ERR "error in registration of network device in the driver");
        x = 1;
        return err;
    }
    printk(KERN_INFO "network device registered successfully, xval = %d", x);
    return 0;
}

static int net_dev_remove(struct platform_device *pdev)
{
    struct net_device *ndev = platform_get_drvdata(pdev);
    unregister_netdev(ndev);
    return 0;
}

static struct platform_driver plat_ndev_driver = {
    .probe = net_dev_probe,
    .remove = net_dev_remove,
    .driver = {
        .name = PLAT_DEVICE_NAME}};

module_platform_driver(plat_ndev_driver);
MODULE_AUTHOR("viveksg");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("diver for chip module");
