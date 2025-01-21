/*
 * Kernel module for Raspberry Pi to communicate with an Arduino that handles SRAM.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/serial.h>
#include <linux/serial_core.h>

#define PROC_NAME "arduinouno"
#define UART_PORT "/dev/ttyACM0"
#define BUFFER_SIZE 256

static char *data_buffer;
static struct file *uart_file;

static ssize_t arduinouno_read(struct file *file, char __user *user_buf, size_t count, loff_t *pos)
{
    char kernel_buf[BUFFER_SIZE];
    ssize_t bytes_read;

    if (*pos > 0) return 0; // EOF

    // Open the UART file
    uart_file = filp_open(UART_PORT, O_RDWR | O_NOCTTY, 0);
    if (IS_ERR(uart_file)) {
        pr_err("Could not open UART file\n");
        return PTR_ERR(uart_file);
    }

    pr_info("Attempting to read from UART...");

    // Wait for UART data to be ready, read from UART
    bytes_read = kernel_read(uart_file, kernel_buf, BUFFER_SIZE - 1, &uart_file->f_pos);
    if (bytes_read < 0) {
        pr_err("Failed to read from UART\n");
        filp_close(uart_file, NULL);
        return bytes_read;
    }

    kernel_buf[bytes_read] = '\0'; // Ensure null termination for safety
    pr_info("Data read from UART: %s\n", kernel_buf);

    // Copy the data read from UART to the user buffer
    if (copy_to_user(user_buf, kernel_buf, bytes_read + 1)) {
        filp_close(uart_file, NULL);
        return -EFAULT;
    }

    *pos += bytes_read;  // Update the position
    filp_close(uart_file, NULL);
    return bytes_read;
}

static ssize_t arduinouno_write(struct file *file, const char __user *user_buf, size_t count, loff_t *pos)
{
    ssize_t bytes_written;

    if (count > BUFFER_SIZE - 1)
        return -EINVAL;

    if (copy_from_user(data_buffer, user_buf, count))
        return -EFAULT;

    data_buffer[count] = '\0';

    uart_file = filp_open(UART_PORT, O_RDWR | O_NOCTTY, 0);
    if (IS_ERR(uart_file)) {
        pr_err("Could not open UART file\n");
        return PTR_ERR(uart_file);
    }

    pr_info("Attempting to write to UART: %s\n", data_buffer);
    bytes_written = kernel_write(uart_file, data_buffer, count, &uart_file->f_pos);
    filp_close(uart_file, NULL);

    if (bytes_written < 0) {
        pr_err("Failed to write to UART\n");
        return bytes_written;
    }

    pr_info("Successfully wrote %zd bytes to UART\n", bytes_written);

    // Add a small delay to allow the UART to process and respond

    return bytes_written;
}

static struct proc_ops proc_ops = {
    .proc_read = arduinouno_read,
    .proc_write = arduinouno_write,
};

static int __init arduinouno_init(void)
{
    data_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!data_buffer)
        return -ENOMEM;

    if (!proc_create(PROC_NAME, 0666, NULL, &proc_ops)) {
        kfree(data_buffer);
        pr_err("Failed to create /proc entry\n");
        return -ENOMEM;
    }

    pr_info("ArduinoUno module loaded\n");
    return 0;
}

static void __exit arduinouno_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    kfree(data_buffer);
    pr_info("ArduinoUno module unloaded\n");
}

module_init(arduinouno_init);
module_exit(arduinouno_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module for Raspberry Pi communication with Arduino");
