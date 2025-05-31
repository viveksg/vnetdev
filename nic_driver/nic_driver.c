
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
    priv->reg_base = (void __iomem *)res->start;//devm_ioremap_resource(&pdev->dev,res);
    printk(KERN_INFO "io base : %x",priv->reg_base);
    if(IS_ERR(priv->reg_base))
    {
        printk(KERN_ERR "Error in fetching reg_base from the resource");
        return PTR_ERR(priv->reg_base);
    }
    printk(KERN_INFO "io base : %x",priv->reg_base);
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

