#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include "dma_controller.h"
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include "registers.h"
#include <linux/workqueue.h>
#include <linux/delay.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt, __func__
uint8_t device_buffer[DEVICE_MEM];
uint8_t device_tx_buffer[DEVICE_MEM];
dev_t ndev_num;
struct cdev ndev;
struct class *ndev_class;
struct device *ndevice;
uint8_t *mem;
uint32_t *reg_base;
uint32_t packet_status = PACKET_NOT_READY;
static struct platform_device *npdev;

dma_descriptor *tx_desc;
dma_descriptor *rx_desc;
static struct workqueue_struct *tx_poll_queue;
static struct delayed_work tx_poll_work;

int queue_op(uint32_t op, dma_descriptor *qbase, uint32_t qhead_ind, uint32_t qtail_ind, uint32_t qsize, uint8_t *buff, uint16_t buff_len, int op_direction, dma_descriptor_fields fields)
{
    uint16_t qtail = reg_base[qtail_ind];
    uint16_t qhead = reg_base[qhead_ind];
    pr_info("queue head %d, queue_tail %d\n", qhead, qtail);
    pr_info("queue base %x",qbase);
    if (op == QUEUE_OP_ENQUEUE)
    {
        if ((qtail + 1) % qsize == qhead)
        {
            pr_info("Queue full, dropping packet\n");
            return QUEUE_FULL;
        }
        pr_info("Attempting to insert packet in queue\n");
        uint8_t *qbuffer = phys_to_virt(qbase[qtail].buffer_address);
        pr_info("queue buffer %x",qbuffer);
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

static void set_mac_address(uint32_t al, uint32_t ah, uint32_t reg_ioffset)
{
    reg_base[REG_RAH + reg_ioffset] = ah;
    reg_base[REG_RAL + reg_ioffset] = al;
}
void init_alloc(void)
{
    mem = kmalloc(REG_MEM_SIZE, GFP_KERNEL);
    memset(mem, 0, REG_MEM_SIZE);
    reg_base = (uint32_t *)mem;
    set_mac_address(MAC_ADDR_AL,MAC_ADD_AH,0);
}

void register_plat_device(void)
{
    phys_addr_t mem_phys = 0;
    mem_phys = virt_to_phys(mem);
    struct resource mem_res[] = {
        {.start = mem_phys,
         .end = mem_phys + REG_MEM_SIZE,
         .flags = IORESOURCE_MEM,
         .name = RES_NAME_REG_BASE}};
    npdev = platform_device_alloc(PLAT_DEVICE_NAME, -1);
    if (!npdev)
    {
        pr_info("platfrom device allocation failed");
        goto free_mem;
    }
    int res_status = platform_device_add_resources(npdev, mem_res, ARRAY_SIZE(mem_res));
    if (res_status)
    {
        pr_info("adding memory address resource to platrom device failed");
        goto put_dev;
    }
    int add_status = platform_device_add(npdev);
    if (add_status)
    {
        pr_info("adding the platform device failed");
        goto put_dev;
    }
    pr_info("platform device added");
    return;
put_dev:
    platform_device_put(npdev);
free_mem:
    kfree(mem);
}

void reset_device(void)
{
    init_alloc();
    reg_base[REG_DEVICE_STATUS] = DEVICE_STATUS_DISABLED;
    register_plat_device();
}

loff_t ndev_lseek(struct file *file_p, loff_t curr_off, int whence)
{
    loff_t temp;
    switch (whence)
    {
    case SEEK_SET:
        temp = curr_off;
        break;
    case SEEK_CUR:
        temp = file_p->f_pos + curr_off;
        break;
    case SEEK_END:
        temp = DEVICE_MEM + curr_off;
        break;
    }

    if (temp > DEVICE_MEM || temp < 0)
        return -EINVAL;
    file_p->f_pos = temp;
    pr_info("New file position: %lld\n", file_p->f_pos);
    return file_p->f_pos;
}

/**
 * todo: corner case to handle if buffer is not fully read and pollling function updates the buffer
 */
ssize_t ndev_read(struct file *filep, char __user *buff, size_t count, loff_t *fpos)
{

    if (reg_base[REG_DEVICE_STATUS] == DEVICE_STATUS_DISABLED)
        return 0;
    if(packet_status == PACKET_NOT_READY)
        return 0;

    if (*fpos + count > DEVICE_MEM)
        count = DEVICE_MEM - (*fpos);

    if (copy_to_user(buff, device_tx_buffer, count))
        return -EFAULT;
    memset(device_tx_buffer,0,count);
    pr_info("Requested file position %x\n", *fpos) ;  
    packet_status = PACKET_NOT_READY;
    *fpos = (*fpos + count)%DEVICE_MEM;
    pr_info("Read %x bytes\n", count);
    pr_info("Updated file position %x\n", *fpos);
    return count;
}

ssize_t ndev_write(struct file *filep, const char __user *buff, size_t count, loff_t *fpos)
{
    // if (*fpos + count > DEVICE_MEM)
    //   count = DEVICE_MEM - (*fpos);
    rx_desc = (dma_descriptor*)((phys_to_virt)((uint64_t)reg_base[REG_RX_RDBAH] << 32 | reg_base[REG_RX_RDBAL]));
    if (reg_base[REG_DEVICE_STATUS] == DEVICE_STATUS_DISABLED)
        return 0;
    if (!count)
    {
        pr_err("Chip module: No memory \n");
        return -ENOMEM;
    }
    if (copy_from_user(device_buffer, buff, count))
    {
        return -EFAULT;
    }

    dma_descriptor_fields temp;
    if (count > SINGLE_BUFFER_MEM)
    {
        int num_queue_ops = count / SINGLE_BUFFER_MEM + (count % SINGLE_BUFFER_MEM > 0 ? 1 : 0);
        int offset = 0;
        int byte_count = 0;
        int remaining_bits = count;
        for (int i = 0; i < num_queue_ops; i++, remaining_bits = remaining_bits - SINGLE_BUFFER_MEM, offset += SINGLE_BUFFER_MEM)
        {
            if(remaining_bits > SINGLE_BUFFER_MEM)
            {
                byte_count = SINGLE_BUFFER_MEM;
                temp.status = 0;
            }
            else
            {
                byte_count = remaining_bits;
                temp.status = RX_STATUS_ENABLE_EOP;
            }
            if (queue_op(QUEUE_OP_ENQUEUE, rx_desc, REG_RX_HEAD, REG_RX_TAIL, DESC_COUNT, &device_buffer[offset], count, 0, temp) == QUEUE_FULL)
            {
                pr_info("Queue full dropping packet\n");
                return -ENOMEM;
            }
        }
    }
    else
    {   
        temp.status = RX_STATUS_ENABLE_EOP;
        if (queue_op(QUEUE_OP_ENQUEUE, rx_desc, REG_RX_HEAD, REG_RX_TAIL, DESC_COUNT, device_buffer, count, 0,temp) == QUEUE_FULL)
        {
            pr_info("Queue full dropping packet\n");
            return -ENOMEM;
        }
    }
    *fpos += count;
    pr_info("Write bytes = %zu\n", count);
    pr_info("Updated file position %lld\n", *fpos);
    return count;
}

int ndev_open(struct inode *inode, struct file *filep)
{
    pr_info("Chip module: open succcessful");
    return 0;
}
int ndev_close(struct inode *inode, struct file *filep)
{
    pr_info("Chip module: close succcessful");
    return 0;
}
struct file_operations ndev_ops = {
    .open = ndev_open,
    .release = ndev_close,
    .read = ndev_read,
    .write = ndev_write,
    .owner = THIS_MODULE
};

static void transmit_polling_function(struct work_struct *work)
{
    if (packet_status == PACKET_NOT_READY)
    {  
        pr_info("settup up packet buffer for transfer");
        uint32_t qhead = reg_base[REG_TX_TDH];
        uint32_t qtail = reg_base[REG_TX_TDT];
        uint32_t qsize = DESC_COUNT;
        dma_descriptor *qbase = (dma_descriptor *)phys_to_virt(((uint64_t)reg_base[REG_TX_TBAH] << 32) | reg_base[REG_TX_TBAL]);
        if (qhead != qtail)
        {
            pr_info("qhead = %x qtail = %x, qbase = %x",qhead,qtail,qbase);
            int offset = 0;
           int i = 0; 
           for (i = qhead; i != qtail; i = (i+1)%qsize)
            {
                uint8_t *buffer_addr = (uint8_t *)phys_to_virt(qbase[i].buffer_address);
                memcpy(&device_tx_buffer[offset], buffer_addr, qbase[i].length);
              //  for(int j = offset; j < qbase[i].length;j++)
                 //   pr_info("device_tx_buffer[%x] = %x",j,device_tx_buffer[j]);

                offset += qbase[i].length;
                if ((qbase[i].status & TX_STATUS_ENABLE_EOP) == TX_STATUS_ENABLE_EOP)
                {  
                    pr_info("packet buffer ready");
                    packet_status = PACKET_READY;
                    reg_base[REG_TX_TDH] = i;
                    break;
                }
            }
        }
    }
    queue_delayed_work(tx_poll_queue,&tx_poll_work,msecs_to_jiffies(DEFAULT_TIMER));
}

static int __init chip_init(void)
{

    int init_status;
    reset_device();
    
    init_status = alloc_chrdev_region(&ndev_num, 0, 1, "plat_net_dev");
    if (init_status < 0)
    {
        pr_err("device num allocation failed\n");
        goto out;
    }
    pr_info("Device major: %d, minor: %d\n", MAJOR(ndev_num), MINOR(ndev_num));
    cdev_init(&ndev, &ndev_ops);
    ndev.owner = THIS_MODULE;
    init_status = cdev_add(&ndev, ndev_num, 1);
    if (init_status < 0)
    {
        pr_err("cannot add the device \n");
        goto unreg_ndev;
    }
    ndev_class = class_create("ndev_class");
    if (IS_ERR(ndev_class))
    {
        pr_err("Class creation failed\n");
        init_status = PTR_ERR(ndev_class);
        goto ndev_del;
    }
    ndevice = device_create(ndev_class, NULL, ndev_num, NULL, "ndevice");
    if (IS_ERR(ndevice))
    {
        pr_err("Device creation failed\n");
        init_status = PTR_ERR(ndevice);
        goto ndev_class_del;
    }

    tx_poll_queue = create_singlethread_workqueue(CARD_WORKQUEUE);
    if(!tx_poll_queue)
    {
        pr_err("Cannot create card workqueue");
        return -ENOMEM;
    }
    INIT_DELAYED_WORK(&tx_poll_work, transmit_polling_function);
    queue_delayed_work(tx_poll_queue,&tx_poll_work,msecs_to_jiffies(DEFAULT_TIMER));
    pr_info("Module init successful \n");
    return 0;
ndev_class_del:
    class_destroy(ndev_class);
ndev_del:
    cdev_del(&ndev);
unreg_ndev:
    unregister_chrdev_region(&ndev_num, 1);
    platform_device_unregister(npdev);
    kfree(mem);
out:
    pr_err("Module initialization failed\n");

    return init_status;
}

static void __exit chip_close(void)
{

    if(tx_poll_queue)
    {
        cancel_delayed_work_sync(&tx_poll_work);
        destroy_workqueue(tx_poll_queue);
    }
    device_destroy(ndev_class, ndev_num);
    class_destroy(ndev_class);
    cdev_del(&ndev);
    unregister_chrdev_region(&ndev_num, 1);
    platform_device_unregister(npdev);
    kfree(mem);
    pr_info("chip module unloaded\n");
}

module_init(chip_init);
module_exit(chip_close);
MODULE_AUTHOR("viveksgt");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("chip module");