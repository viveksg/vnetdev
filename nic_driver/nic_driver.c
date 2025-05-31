
#include "dma_controller.h"
#include "registers.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
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

struct net_device *net_pdev;
struct pndev_priv{
    void __iomem *reg_base;
    int tx_irq;
    int rx_irq;
    struct net_device *ndev;
};

dma_descriptor *tx_desc;
dma_descriptor *rx_desc;
uint32_t *reg_base;
uint8_t *tx_packet_buff;
uint8_t *rx_packet_buff;
uint8_t *mem;

int queue_op(uint32_t op, dma_descriptor *qbase, uint32_t qhead_ind, uint32_t qtail_ind, uint32_t qsize, uint8_t *buff, uint16_t buff_len, int op_direction)
{
    uint16_t qtail = reg_base[qtail_ind];
    uint16_t qhead = reg_base[qhead_ind];
    pr_info("queue head %d, queue_tail %d\n",qhead,qtail);
    if (op == QUEUE_OP_ENQUEUE)
    {
        if ((qtail + 1) % qsize == qhead)
        {
            pr_info("Queue full, dropping packet\n");
            return QUEUE_FULL;
        }
        pr_info("Attempting to insert packet in queue\n");
        uint8_t *qbuffer = phys_to_virt(qbase[qtail].buffer_address);
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
    uint64_t tx_base = (uint64_t)((void *)tx_desc);
    uint64_t rx_base = (uint64_t)((void *)rx_desc);

    reg_base[REG_RX_RDBAL] = rx_base & (~(((uint64_t)(0xFFFFFFFF)) << 32));
    reg_base[REG_RX_RDBAH] = rx_base >> 32;
    reg_base[REG_RX_HEAD] = 0;
    reg_base[REG_RX_TAIL] = 0;
    reg_base[REG_RX_RDLEN] = DESC_COUNT;

    reg_base[REG_TX_TBAL] = tx_base & (~(((uint64_t)(0xFFFFFFFF)) << 32));
    reg_base[REG_TX_TBAH] = tx_base >> 32;
    reg_base[REG_TX_TDH] = 0;
    reg_base[REG_TX_TDT] = 0;
    reg_base[REG_TX_TDLEN] = DESC_COUNT;
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
        pr_info("Descriptor base = %x,\n", base[i].buffer_address);
    }
}

void init_alloc(void)
{
    mem = kmalloc(TOTAL_MEM , GFP_KERNEL);
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

static int net_open(struct net_device* sndev )
{
    return 0;
}

static int net_close(struct net_device* sndev)
{
    return 0;
}

static netdev_tx_t net_xmit(struct sk_buff *sbuff, struct net_device *sndev)
{
    return NETDEV_TX_OK;
}

static const struct net_device_ops net_ops = {
    .ndo_open = net_open,
    .ndo_stop = net_close,
    .ndo_start_xmit = net_xmit
};

static void net_dev_setup(struct net_device *ndev)
{
    /*printk(KERN_INFO "setting up network device");
    static char ndev_mac_addr ;// todo get this from device
    ndev->netdev_ops = &net_ops;
    ndev->addr_len = ETH_ALEN;
    ndev->type = ARPHRD_ETHER;
    eth_hw_addr_set(ndev, ndev_mac_addr);
    ndev->mtu = MTU;
    ndev->flags |= IFF_BROADCAST | IFF_MULTICAST;
    printk(KERN_INFO "Net device setup complete");*/
}

static int init_net_device(void)
{
    net_pdev = alloc_netdev(0,"nic%d", NET_NAME_UNKNOWN, net_dev_setup);
    if(!net_pdev)
    {
        printk(KERN_ERR "Network device allocation failed");
        return -ENOMEM;
    }
    if(register_netdev(net_pdev))
    {
        printk(KERN_ERR "Device registration failed");
        free_netdev(net_pdev);
        return -1;
    }
    printk(KERN_INFO "Device successfully registered");
    return 0;

}

static int net_dev_probe(struct platform_device *pdev)
{
    printk("probe function called\n");

    struct pndev_priv *priv;
    struct resource *res;
    net_pdev = alloc_etherdev(sizeof(struct pndev_priv));
    if(!net_pdev)
    {
        printk(KERN_ERR "allocation of nety device failed");
        return -ENOMEM;
    }
    priv = netdev_priv(net_pdev);
    platform_set_drvdata(pdev, net_pdev);

    res = platform_get_resource(pdev, IORESOURCE_MEM,0);
    priv->reg_base = (void __iomem *)phys_to_virt(res->start);//devm_ioremap_resource(&pdev->dev,res);
    printk(KERN_INFO "io base : %x",priv->reg_base);
    if(IS_ERR(priv->reg_base))
    {
        printk(KERN_ERR "Error in fetching reg_base from the resource");
        return PTR_ERR(priv->reg_base);
    }
    printk(KERN_INFO "io base : %x",priv->reg_base);
    reg_base = (uint32_t*)priv->reg_base;
    printk(KERN_INFO " reg_base %x", reg_base);
    reset_device();
    net_pdev->netdev_ops = &net_ops;
    priv->ndev = net_pdev;
    eth_hw_addr_random(net_pdev);
    int err = register_netdev(net_pdev);
    if(!err)
    {
        return err;
    }
    printk(KERN_INFO "network device registered successfully");
    return 0;
}

static int net_dev_remove(struct platform_device *pdev)
{
   // struct net_device *ndev = platform_get_drvdata(pdev);
  //  struct vnic_priv *priv = netdev_priv(ndev);

  //  unregister_netdev(ndev);
    struct net_device *ndev = platform_get_drvdata(pdev);
    unregister_netdev(ndev);
    return 0;
}


static struct platform_driver plat_ndev_driver = {
    .probe = net_dev_probe,
    .remove = net_dev_remove,
    .driver = {
        .name = PLAT_DEVICE_NAME
    }
};

/*static int __init plat_ndev_driver_init(void)
{
    platform_driver_register(&plat_ndev_driver);
    return 0;
    
}
static void __exit plat_ndev_driver_exit(void)
{

}*/
module_platform_driver(plat_ndev_driver);
MODULE_AUTHOR("viveksg");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("diver for chip module");

