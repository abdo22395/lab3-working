#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

/*
Load the module with
sudo insmod kernelskoj2.ko

Unload the module with
sudo rmmod kernelskoj2.ko

Try reading with "cat /proc/arduinouno"
Try writing with  "echo 'hello' >> /proc/arduinouno"

Read the kernel module output with
sudo dmesg | tail
*/

#define MESSAGE_LENGTH 5
static struct proc_dir_entry* proc_entry;
#define PROCFS_MAX_SIZE     20
static char read_proc_buffer[PROCFS_MAX_SIZE];
static unsigned long read_proc_buffer_size = 0;


static ssize_t read_proc(struct file *file, char __user *user_buffer,
                         size_t count, loff_t *offset)
{
    printk(KERN_INFO "Called read_proc\n");

    /* Return 0 (EOF) if already read or if count is too small for buffer */
    if (*offset > 0 || count < PROCFS_MAX_SIZE) {
        return 0;
    }

    /* Open the UART device */
    struct file* uart_file = filp_open("/dev/ttyACM0", O_RDONLY, 0);
    if (IS_ERR(uart_file)) {
        printk(KERN_ERR "Failed to open UART device: %ld\n", PTR_ERR(uart_file));
        return -EFAULT;
    }

    /* Clear any leftover data in the UART buffer */
    ssize_t nread = 0;
    char buf[PROCFS_MAX_SIZE] = {0};

    /* Optionally, clear out any previous data */
    if (kernel_read(uart_file, buf, PROCFS_MAX_SIZE, &uart_file->f_pos) < 0) {
        printk(KERN_ERR "Failed to read from UART device.\n");
        filp_close(uart_file, NULL);
        return -EFAULT;
    }

    /* Read new data from the UART device */
    nread = kernel_read(uart_file, buf, PROCFS_MAX_SIZE, &uart_file->f_pos);

    /* Close the UART file */
    filp_close(uart_file, NULL);

    if (nread < 0) {
        printk(KERN_ERR "Error reading from UART device: %zd\n", nread);
        return -EFAULT;
    }

    /* Copy the data from the kernel buffer to user space */
    if (copy_to_user(user_buffer, buf, nread)) {
        return -EFAULT;
    }

    /* Update the offset so subsequent reads return EOF */
    *offset = nread;

    /* Return the number of bytes read */
    return nread;
}


static ssize_t write_proc(struct file* file, const char __user* user_buffer,
                          size_t count, loff_t* offset)
{
    printk(KERN_INFO "Called write_proc\n");

    char *tmp = kzalloc(count + 1, GFP_KERNEL);
    if (!tmp)
        return -ENOMEM;

    if (copy_from_user(tmp, user_buffer, count)) {
        kfree(tmp);
        return -EFAULT;
    }

        // Start of Arduino Write thingy
        struct file* testfd = filp_open("/dev/ttyACM0",O_RDWR,0);


        ssize_t wr = kernel_write(testfd, tmp, count + 1, offset);

        filp_close(testfd, NULL);

        // End of Arduino Write thingy*/

    size_t write_len = (count > PROCFS_MAX_SIZE) ? PROCFS_MAX_SIZE : count;
    memcpy(read_proc_buffer, tmp, write_len);
    read_proc_buffer_size = write_len;

    kfree(tmp);


    return write_len;
}


static const struct proc_ops hello_proc_fops = {
  .proc_read = read_proc,
  .proc_write = write_proc,
};

static int __init construct(void) {
        proc_entry = proc_create("arduinouno", 0666, NULL, &hello_proc_fops);
        pr_info("Loading 'arduinouno' module!\n");
        return 0;
}



static void __exit destruct(void) {
        proc_remove(proc_entry);
        pr_info("First kernel module has been removed\n");
}



module_init(construct);
module_exit(destruct);

MODULE_LICENSE("GPL");
