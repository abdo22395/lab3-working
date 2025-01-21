#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/random.h>

#define PROCFS_MAX_SIZE  20
#define RANDOM_DATA_SIZE 10

static char read_proc_buffer[PROCFS_MAX_SIZE];
static unsigned long read_proc_buffer_size = 0;

// Function to generate random data and write it to the UART device
static ssize_t write_proc(struct file *file, const char __user *user_buffer,
                          size_t count, loff_t *offset)
{
    struct file *uart_file = NULL;
    ssize_t bytes_written = 0;
    char random_data[RANDOM_DATA_SIZE];
    struct tty_struct *tty = NULL;

    printk(KERN_INFO "Called write_proc\n");

    // Generate random data
    get_random_bytes(random_data, RANDOM_DATA_SIZE);

    // Open the UART device for writing
    uart_file = filp_open("/dev/ttyACM0", O_WRONLY, 0);
    if (IS_ERR(uart_file)) {
        long err_code = PTR_ERR(uart_file);
        printk(KERN_ERR "Failed to open UART device for writing: %ld\n", err_code);
        return err_code;
    }

    // Get the tty struct from the UART device file
    tty = uart_file->f_inode->i_private;

    // Check if the tty driver supports write operations
    if (tty->driver && tty->driver->ops && tty->driver->ops->write) {
        // Write the random data to the UART device
        bytes_written = tty->driver->ops->write(tty, random_data, RANDOM_DATA_SIZE);
        if (bytes_written < 0) {
            printk(KERN_ERR "Failed to write random data to UART device: %ld\n", bytes_written);
            filp_close(uart_file, NULL);
            return bytes_written;
        }
    } else {
        printk(KERN_ERR "No valid tty write operation available\n");
        filp_close(uart_file, NULL);
        return -EINVAL;
    }

    printk(KERN_INFO "Written random data to UART: %.*s\n", RANDOM_DATA_SIZE, random_data);

    // Close the UART device after writing
    filp_close(uart_file, NULL);

    return bytes_written;
}

// Function to read data from the UART device and copy it to user-space
static ssize_t read_proc(struct file *file, char __user *user_buffer,
                         size_t count, loff_t *offset)
{
    struct file* uart_file = NULL;
    struct tty_struct *tty = NULL;
    ssize_t bytes_read = 0;

    printk(KERN_INFO "Called read_proc\n");

    // Return 0 (EOF) if already read or if count is too small for buffer
    if (*offset > 0 || count < PROCFS_MAX_SIZE) {
        return 0;
    }

    // Open the UART device for reading
    uart_file = filp_open("/dev/ttyACM0", O_RDONLY, 0);
    if (IS_ERR(uart_file)) {
        long err_code = PTR_ERR(uart_file);
        printk(KERN_ERR "Failed to open UART device: %ld\n", err_code);
        return err_code;
    }

    // Get the tty struct associated with the UART device
    tty = uart_file->f_inode->i_private;
    if (!tty) {
        printk(KERN_ERR "UART device does not have an associated tty struct\n");
        filp_close(uart_file, NULL);
        return -ENODEV;
    }

    // Check if the tty driver supports read operations
    if (tty->driver && tty->driver->ops && tty->driver->ops->read) {
        // Read data directly from the UART device
        bytes_read = tty->driver->ops->read(tty, read_proc_buffer, PROCFS_MAX_SIZE);

        // Check for errors in reading
        if (bytes_read < 0) {
            printk(KERN_ERR "Failed to read from UART device via tty: %ld\n", bytes_read);
            filp_close(uart_file, NULL);
            return bytes_read;
        }
    } else {
        printk(KERN_ERR "No valid tty read operation available\n");
        filp_close(uart_file, NULL);
        return -EINVAL;
    }

    // Close the UART device after reading
    filp_close(uart_file, NULL);

    // Copy the data from the kernel buffer to user-space
    if (copy_to_user(user_buffer, read_proc_buffer, bytes_read)) {
        printk(KERN_ERR "Failed to copy data to user-space\n");
        return -EFAULT;
    }

    // Update the offset to return EOF for subsequent reads
    *offset = bytes_read;

    // Return the number of bytes read
    return bytes_read;
}

// Define the file operations structure
static const struct proc_ops hello_proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,  // Add support for writing
};

// Module initialization
static int __init construct(void) {
    proc_create("arduinouno", 0666, NULL, &hello_proc_fops);
    pr_info("Loading 'arduinouno' module!\n");
    return 0;
}

// Module cleanup
static void __exit destruct(void) {
    proc_remove(proc_entry);
    pr_info("Module 'arduinouno' has been removed\n");
}

module_init(construct);
module_exit(destruct);

MODULE_LICENSE("GPL");
