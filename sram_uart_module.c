#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>   // For kzalloc

#define PROCFS_MAX_SIZE     1024  // Increase buffer size
#define UART_DEVICE_PATH    "/dev/ttyACM0"  // UART device

static struct proc_dir_entry* proc_entry;

static ssize_t read_proc(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    printk(KERN_INFO "Called read_proc\n");

    // If we've already read (offset > 0) or the requested size is too small, return EOF
    if (*offset > 0 || count < PROCFS_MAX_SIZE) {
        return 0;
    }

    // Open the UART device
    struct file* uart_file = filp_open(UART_DEVICE_PATH, O_RDONLY, 0);
    if (IS_ERR(uart_file)) {
        printk(KERN_ERR "Failed to open UART device %s: %ld\n", UART_DEVICE_PATH, PTR_ERR(uart_file));
        return -EFAULT;
    }

    // Dynamically allocate a larger buffer for reading data
    char *uart_buffer = kzalloc(PROCFS_MAX_SIZE, GFP_KERNEL);
    if (!uart_buffer) {
        filp_close(uart_file, NULL);
        return -ENOMEM;
    }

    // Read data from the UART
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

    // Copy the read data to user-space
    if (copy_to_user(user_buffer, uart_buffer, total_read)) {
        kfree(uart_buffer);
        return -EFAULT;
    }

    // Update offset so the next read can return EOF if no more data is available
    *offset = total_read;

    // Free the allocated buffer
    kfree(uart_buffer);

    // Return the number of bytes we read and sent to user space
    return total_read;
}

static ssize_t write_proc(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{
    printk(KERN_INFO "Called write_proc\n");

    char *tmp = kzalloc(count + 1, GFP_KERNEL);
    if (!tmp)
        return -ENOMEM;

    if (copy_from_user(tmp, user_buffer, count)) {
        kfree(tmp);
        return -EFAULT;
    }

    // Open UART device for writing
    struct file* uart_file = filp_open(UART_DEVICE_PATH, O_WRONLY, 0);
    if (IS_ERR(uart_file)) {
        kfree(tmp);
        printk(KERN_ERR "Failed to open UART device %s: %ld\n", UART_DEVICE_PATH, PTR_ERR(uart_file));
        return -EFAULT;
    }

    // Write to UART device
    ssize_t bytes_written = kernel_write(uart_file, tmp, count, &uart_file->f_pos);

    filp_close(uart_file, NULL);

    // Free the temporary buffer
    kfree(tmp);

    if (bytes_written < 0) {
        return bytes_written;
    }

    // Return the number of bytes written
    return bytes_written;
}

static const struct proc_ops hello_proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

static int __init construct(void)
{
    // Create the /proc entry
    proc_entry = proc_create("arduinouno", 0666, NULL, &hello_proc_fops);
    if (!proc_entry) {
        printk(KERN_ERR "Failed to create /proc/arduinouno\n");
        return -ENOMEM;
    }

    pr_info("Loading 'arduinouno' module!\n");
    return 0;
}

static void __exit destruct(void)
{
    // Remove the /proc entry
    proc_remove(proc_entry);
    pr_info("Kernel module removed\n");
}

module_init(construct);
module_exit(destruct);

MODULE_LICENSE("GPL");
