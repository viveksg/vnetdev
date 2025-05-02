
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt, __func__
#define DEVICE_MEM 8096
char device_buffer[DEVICE_MEM];
dev_t ndev_num;
struct cdev ndev;
struct class *ndev_class;
struct device *ndevice;
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
    if(*fpos + count > DEVICE_MEM)
        count = DEVICE_MEM - (*fpos);
    
    if(copy_to_user(buff,&device_buffer[*fpos], count))
        return -EFAULT;

    *fpos += count;
    pr_info("Read %n bytes = %zu\n",count);
    pr_info("Updated file position %lld\n",fpos);
    return count;
}

ssize_t ndev_write(struct file *filep, const char __user *buff, size_t count, loff_t *fpos)
{
    if(*fpos + count > DEVICE_MEM)
        count = DEVICE_MEM - (*fpos);
    if(!count)
    {
        pr_err("Chip module: No memory \n");
        return -ENOMEM;
    }
    if(copy_from_user(&device_buffer[*fpos],buff,count))
    {
        return -EFAULT;
    }
    *fpos += count;
    pr_info("Write bytes = %zu\n",count);
    pr_info("Updated file position %lld\n",*fpos);
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
struct file_operations ndev_ops={
    .open = ndev_open,
    .release = ndev_close,
    .read = ndev_read,
    .write = ndev_write,
    .owner = THIS_MODULE
};

static int __init chip_init(void)
{

    int init_status;
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
    device_destroy(ndev_class,ndev_num);
    class_destroy(ndev_class);
    cdev_del(&ndev);    
    unregister_chrdev_region(&ndev_num, 1);
    pr_info("modile unloaded\n");
}

module_init(chip_init);
module_exit(chip_close);
MODULE_AUTHOR("viveksgt");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("chip module");