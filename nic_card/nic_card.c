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
#include "registers.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt, __func__
#define DEVICE_MEM 8096
char device_buffer[DEVICE_MEM];
dev_t ndev_num;
struct cdev ndev;
struct class *ndev_class;
struct device *ndevice;

dma_descriptor *tx_desc;
dma_descriptor *rx_desc;
uint32_t *reg_base;
uint8_t *tx_packet_buff;
uint8_t *rx_packet_buff;
#define REG_MEM_SIZE 4096
#define TX_DESC_MEM (DESC_COUNT * DESC_SIZE)
#define RX_DESC_MEM (DESC_COUNT * DESC_SIZE)
#define TX_BUFFER_MEM (MTU * DESC_COUNT)
#define RX_BUFFER_MEM (MTU * DESC_COUNT)
#define TOTAL_MEM (REG_MEM_SIZE + TX_DESC_MEM + RX_DESC_MEM + TX_BUFFER_MEM + RX_BUFFER_MEM)
#define REG_OFFSET 0
#define TX_DESC_MEM_OFFSET (REG_MEM_SIZE)
#define RX_DESC_MEM_OFFSET (TX_DESC_MEM_OFFSET + TX_DESC_MEM)
#define TX_BUFFER_OFFSET (RX_DESC_MEM_OFFSET + RX_DESC_MEM)
#define RX_BUFFER_OFFSET (TX_BUFFER_OFFSET + TX_BUFFER_MEM)
uint8_t *mem;

int queue_op(uint32_t op, dma_descriptor *qbase, uint32_t qhead_ind, uint32_t qtail_ind, uint32_t qsize, uint8_t *buff, uint16_t buff_len, int op_direction)
{
    uint16_t qtail = reg_base[qtail_ind];
    uint16_t qhead = reg_base[qhead_ind];
    if (op == QUEUE_OP_ENQUEUE)
    {
        if ((qtail + 1) % qsize == qhead)
        {
            printk("Queue full, dropping packet\n");
            return QUEUE_FULL;
        }
        uint8_t *qbuffer = phy_to_virt(qbase[qtail].buffer_address);
        if (op_direction == COPY_FROM_QUEUE)
            memcpy(buff, qbuffer, qbase[qtail].length);
        else
        {
            memcpy(qbuffer, buff, buff_len);
            qbase[qtail].length = buff_len
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

void reset_desc_registers()
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
        printk("Descriptor base = %x,\n", base[i].buffer_address);
    }
}

void init_alloc(void)
{
    mem = kmalloc(TOTAL_MEM, GFP_KERNEL);
    memset(mem, 0, TOTAL_MEM);
    reg_base = (uint32_t *)mem;
    tx_desc = (dma_descriptor *)((char *)mem + TX_DESC_MEM_OFFSET);
    rx_desc = (dma_descriptor *)((char *)mem + RX_DESC_MEM_OFFSET);
    tx_packet_buff = (char *)((char *)mem + TX_BUFFER_OFFSET);
    rx_packet_buff = (char *)((char *)mem + RX_BUFFER_OFFSET);

    printk("Allocated %ld bytes at address %p", TOTAL_MEM, (char *)mem);
    printk("Address details:\n");
    printk("Address reg_base: %x\n", (uint64_t)reg_base);
    printk("Address tx_desc: %x\n", ((uint64_t)tx_desc));
    printk("Address tx_desc:- %x\n", (((uint64_t)mem + 4096)));
    printk("Address rx_desc: %x\n", (uint64_t)rx_desc);
    printk("Address tx_packet_buff: %x\n", (uint64_t)tx_packet_buff);
    printk("Address rx_packet_buff: %x\n", (uint64_t)rx_packet_buff);
}

void reset_device()
{
    init_alloc();
    init_descriptors(tx_desc, MTU, tx_packet_buff, DESC_COUNT);
    init_descriptors(rx_desc, MTU, tx_packet_buff, DESC_COUNT);
    print_descriptors(tx_desc, DESC_COUNT);
    print_descriptors(rx_desc, DESC_COUNT);
    reset_desc_registers();
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

ssize_t ndev_read(struct file *filep, char __user *buff, size_t count, loff_t *fpos)
{
    if (*fpos + count > DEVICE_MEM)
        count = DEVICE_MEM - (*fpos);

    if (copy_to_user(buff, &device_buffer[*fpos], count))
        return -EFAULT;

    *fpos += count;
    pr_info("Read %n bytes = %zu\n", count);
    pr_info("Updated file position %lld\n", fpos);
    return count;
}

ssize_t ndev_write(struct file *filep, const char __user *buff, size_t count, loff_t *fpos)
{
    if (*fpos + count > DEVICE_MEM)
        count = DEVICE_MEM - (*fpos);
    if (!count)
    {
        pr_err("Chip module: No memory \n");
        return -ENOMEM;
    }
    if (copy_from_user(&device_buffer[*fpos], buff, count))
    {
        return -EFAULT;
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
    .owner = THIS_MODULE};

static int __init chip_init(void)
{

    int init_status;
    init_alloc();
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
    pr_info("Module init successful \n");
    return 0;
ndev_class_del:
    class_destroy(ndev_class);
ndev_del:
    cdev_del(&ndev);
unreg_ndev:
    unregister_chrdev_region(&ndev_num, 1);
out:
    pr_err("Module initialization failed\n");

    return init_status;
}

static void __exit chip_close(void)
{
    device_destroy(ndev_class, ndev_num);
    class_destroy(ndev_class);
    cdev_del(&ndev);
    unregister_chrdev_region(&ndev_num, 1);
    pr_info("module unloaded\n");
}

module_init(chip_init);
module_exit(chip_close);
MODULE_AUTHOR("viveksgt");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("chip module");