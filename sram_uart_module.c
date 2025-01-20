#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/errno.h>
#include <linux/string.h>

#define DEVICE_NAME "sram_uart_device"
#define UART_DEVICE "/dev/ttyAMA0" // Adjust as needed based on your device

static struct proc_dir_entry *proc_entry;
static char data_buffer[256];  // Buffer for storing read or written data

// Function to send data to UART
static void uart_send_data(struct tty_struct *tty, const char *data)
{
    while (*data) {
        tty->driver->ops->write(tty, data, 1);  // Send one byte at a time
        data++;
    }
}

// Function to receive data from UART
static void uart_receive_data(struct tty_struct *tty)
{
    int i = 0;
    unsigned char ch;
    memset(data_buffer, 0, sizeof(data_buffer));

    // Read one byte at a time from UART
    while (tty->driver->ops->read(tty, &ch, 1) > 0 && i < sizeof(data_buffer) - 1) {
        data_buffer[i++] = ch;
    }
    data_buffer[i] = '\0';  // Null-terminate the string
}

// File operations for reading from /proc
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    struct tty_struct *tty;
    tty = tty_get_by_name(UART_DEVICE);  // Get the UART device by its name

    if (tty == NULL) {
        pr_err("Failed to get UART device: %s\n", UART_DEVICE);
        return -ENODEV;  // Return error if UART device not found
    }

    uart_receive_data(tty);  // Read data from UART

    if (*offset > 0) {
        tty_put(tty);  // Release tty if we've already read
        return 0;
    }

    // Copy the data buffer to user space
    if (copy_to_user(buf, data_buffer, strlen(data_buffer))) {
        tty_put(tty);  // Release tty on failure
        return -EFAULT;
    }

    *offset += strlen(data_buffer);  // Update the offset
    tty_put(tty);  // Release tty after use
    return strlen(data_buffer);
}

// File operations for writing to /proc
static ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    struct tty_struct *tty;
    tty = tty_get_by_name(UART_DEVICE);  // Get the UART device by its name

    if (count > sizeof(data_buffer) - 1) {
        tty_put(tty);  // Release tty if the data is too large
        return -EINVAL;
    }

    // Copy the user input to kernel space
    if (copy_from_user(data_buffer, buf, count)) {
        tty_put(tty);  // Release tty on failure
        return -EFAULT;
    }

    data_buffer[count] = '\0';  // Null-terminate the string

    uart_send_data(tty, data_buffer);  // Send data to UART
    tty_put(tty);  // Release tty after use

    return count;
}

// File operations structure for /proc file handling
static struct file_operations fops = {
    .read = proc_read,
    .write = proc_write,
};

// Module initialization function
static int __init sram_uart_module_init(void)
{
    // Create a /proc entry for our module
    proc_entry = proc_create(DEVICE_NAME, 0666, NULL, &fops);
    if (!proc_entry) {
        pr_err("Failed to create /proc entry\n");
        return -ENOMEM;  // Return error if /proc entry creation fails
    }

    pr_info("SRAM UART Module Initialized\n");
    return 0;
}

// Module cleanup function
static void __exit sram_uart_module_exit(void)
{
    // Remove the /proc entry
    if (proc_entry) {
        proc_remove(proc_entry);
    }

    pr_info("SRAM UART Module Exited\n");
}

module_init(sram_uart_module_init);
module_exit(sram_uart_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module to interface with SRAM 23LCV512 via UART on Raspberry Pi");
