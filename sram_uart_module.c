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



static ssize_t read_proc(struct file *file, char __user *user_buffer,
                         size_t count, loff_t *offset)
{
    printk(KERN_INFO "Called read_proc\n");

    // Return 0 (EOF) if already read or if count is too small for buffer
    if (*offset > 0 || count < PROCFS_MAX_SIZE) {
        return 0;
    }

    // Open the UART device
    struct file* uart_file = filp_open("/dev/ttyACM0", O_RDONLY, 0);
    if (IS_ERR(uart_file)) {
        printk(KERN_ERR "Failed to open UART device %ld\n", PTR_ERR(uart_file));
        return -EFAULT;
    }

    // Clear any leftover data in the UART buffer by reading a chunk of data
    char discard_buffer[PROCFS_MAX_SIZE] = {0};
    kernel_read(uart_file, discard_buffer, PROCFS_MAX_SIZE, &uart_file->f_pos);  // Discard data

    // Now read fresh data from the UART
    char uart_buffer[PROCFS_MAX_SIZE] = {0};
    ssize_t total_read = 0;
    ssize_t bytes_read;
    while (total_read < PROCFS_MAX_SIZE) {
        bytes_read = kernel_read(uart_file, uart_buffer + total_read, PROCFS_MAX_SIZE - total_read, &uart_file->f_pos);
        if (bytes_read <= 0) {
            break;  // Exit if no more data is available
        }
        total_read += bytes_read;
    }

    filp_close(uart_file, NULL);

    // Copy the data from the kernel buffer to user-space
    if (copy_to_user(user_buffer, uart_buffer, total_read)) {
        return -EFAULT;
    }

    // Update the offset to return EOF for subsequent reads
    *offset = total_read;

    // Return the number of bytes read
    return total_read;
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
