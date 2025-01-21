#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/termios.h>

#define PROCFS_MAX_SIZE 20
#define UART_DEV "/dev/ttyACM0"  // UART device for Arduino

static struct proc_dir_entry* proc_entry;
static char read_proc_buffer[PROCFS_MAX_SIZE];
static unsigned long read_proc_buffer_size = 0;

// Function to open the UART device
static struct file* open_uart_device(void) {
    struct file* uart_file = filp_open(UART_DEV, O_RDWR, 0);
    if (IS_ERR(uart_file)) {
        printk(KERN_ERR "Failed to open UART device %ld\n", PTR_ERR(uart_file));
        return NULL;
    }
    return uart_file;
}

// Function to write data to the UART device
static ssize_t write_to_uart(struct file* uart_file, const char *data, size_t len) {
    ssize_t bytes_written = kernel_write(uart_file, data, len, &uart_file->f_pos);
    return bytes_written;
}

// Function to read data from the UART device
static ssize_t read_from_uart(struct file* uart_file, char *buffer, size_t len) {
    ssize_t bytes_read = kernel_read(uart_file, buffer, len, &uart_file->f_pos);
    return bytes_read;
}

// The read function for the proc file
static ssize_t read_proc(struct file *file, char __user *user_buffer,
                         size_t count, loff_t *offset) {
    printk(KERN_INFO "Called read_proc\n");

    // Return EOF if already read
    if (*offset > 0) {
        return 0;
    }

    struct file* uart_file = open_uart_device();
    if (!uart_file) {
        return -EFAULT;
    }

    // Send a command to the Arduino to read data from SRAM
    const char *command = "READ";
    ssize_t written = write_to_uart(uart_file, command, strlen(command));
    if (written < 0) {
        filp_close(uart_file, NULL);
        return -EFAULT;
    }

    // Read the response from the Arduino (the data from SRAM)
    ssize_t bytes_read = read_from_uart(uart_file, read_proc_buffer, sizeof(read_proc_buffer));
    if (bytes_read < 0) {
        filp_close(uart_file, NULL);
        return -EFAULT;
    }

    // Copy the data to user space
    if (copy_to_user(user_buffer, read_proc_buffer, bytes_read)) {
        filp_close(uart_file, NULL);
        return -EFAULT;
    }

    *offset = bytes_read;
    filp_close(uart_file, NULL);
    return bytes_read;
}

// The write function for the proc file
static ssize_t write_proc(struct file* file, const char __user* user_buffer,
                          size_t count, loff_t* offset) {
    printk(KERN_INFO "Called write_proc\n");

    char buffer[PROCFS_MAX_SIZE];
    if (count > PROCFS_MAX_SIZE) {
        count = PROCFS_MAX_SIZE;
    }

    // Copy data from user space
    if (copy_from_user(buffer, user_buffer, count)) {
        return -EFAULT;
    }

    // Open the UART device
    struct file* uart_file = open_uart_device();
    if (!uart_file) {
        return -EFAULT;
    }

    // Send data to the Arduino via UART
    ssize_t bytes_written = write_to_uart(uart_file, buffer, count);
    if (bytes_written < 0) {
        filp_close(uart_file, NULL);
        return -EFAULT;
    }

    filp_close(uart_file, NULL);
    return count;
}

static const struct proc_ops hello_proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

static int __init construct(void) {
    // Create the /proc/arduinouno entry
    proc_entry = proc_create("arduinouno", 0666, NULL, &hello_proc_fops);
    if (!proc_entry) {
        printk(KERN_ERR "Failed to create /proc/arduinouno\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "Loading 'arduinouno' module!\n");
    return 0;
}

static void __exit destruct(void) {
    // Remove the proc entry and cleanup
    proc_remove(proc_entry);
    printk(KERN_INFO "Removing 'arduinouno' module\n");
}

module_init(construct);
module_exit(destruct);

MODULE_LICENSE("GPL");
