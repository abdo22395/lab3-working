#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

#define DEVICE_NAME "sram_uart_device"

static struct uart_port *uart_port = NULL;
static struct proc_dir_entry *proc_entry;
static char data_buffer[256];

// Funktion för att skicka data till Arduino via UART
static void uart_send_data(const char *data)
{
    while (*data) {
        // Skriv varje tecken till UART-porten
        uart_putc(uart_port, *data++);
    }
}

// Funktion för att läsa data från Arduino via UART
static void uart_receive_data(void)
{
    int ch;
    int i = 0;
    memset(data_buffer, 0, sizeof(data_buffer));

    // Läs data från UART
    while ((ch = uart_getc(uart_port)) != -1 && i < sizeof(data_buffer) - 1) {
        data_buffer[i++] = ch;
    }
    data_buffer[i] = '\0'; // Null-terminera strängen
}

// PROC filoperationer
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    uart_receive_data();  // Läs data från UART
    if (*offset > 0) {
        return 0;
    }
    if (copy_to_user(buf, data_buffer, strlen(data_buffer))) {
        return -EFAULT;
    }
    *offset += strlen(data_buffer);
    return strlen(data_buffer);
}

static ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    if (count > sizeof(data_buffer) - 1) {
        return -EINVAL;
    }
    if (copy_from_user(data_buffer, buf, count)) {
        return -EFAULT;
    }
    data_buffer[count] = '\0';
    uart_send_data(data_buffer);  // Skicka data via UART
    return count;
}

// File operations
static struct file_operations fops = {
    .read = proc_read,
    .write = proc_write,
};

// Modul init
static int __init sram_uart_module_init(void)
{
    uart_port = uart_get_by_id(0); // Antag att vi använder UART0

    if (!uart_port) {
        pr_err("UART port not found\n");
        return -ENODEV;
    }

    // Skapa en PROC-fil
    proc_entry = proc_create(DEVICE_NAME, 0666, NULL, &fops);
    if (!proc_entry) {
        pr_err("Failed to create /proc entry\n");
        return -ENOMEM;
    }

    pr_info("SRAM UART Module Initialized\n");
    return 0;
}

// Modul exit
static void __exit sram_uart_module_exit(void)
{
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
