#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/string.h>

#define DEVICE_NAME "sram_uart_device"

static struct proc_dir_entry *proc_entry;
static char data_buffer[256];  // Buffert för att lagra läst eller skrivet data

// Funktion för att skicka data till UART
static void uart_send_data(struct tty_struct *tty, const char *data)
{
    while (*data) {
        tty->driver->ops->write(tty, data, 1);
        data++;
    }
}

// Funktion för att läsa data från UART
static void uart_receive_data(struct tty_struct *tty)
{
    int i = 0;
    unsigned char ch;
    memset(data_buffer, 0, sizeof(data_buffer));

    while (tty->driver->ops->read(tty, &ch, 1) > 0 && i < sizeof(data_buffer) - 1) {
        data_buffer[i++] = ch;
    }
    data_buffer[i] = '\0';  // Null-terminera strängen
}

// PROC filoperationer
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    struct tty_struct *tty = NULL;

    // Hämta tty för den aktuella UART-porten (här använd UART0 som exempel)
    tty = tty_get_by_name("ttyAMA0");

    if (tty) {
        uart_receive_data(tty);  // Läs data från UART
        if (*offset > 0) {
            tty_put(tty);  // Släpp tty
            return 0;
        }
        if (copy_to_user(buf, data_buffer, strlen(data_buffer))) {
            tty_put(tty);  // Släpp tty vid fel
            return -EFAULT;  // Om kopiering misslyckas, returnera fel
        }
        *offset += strlen(data_buffer);  // Uppdatera offset för nästa läsning
        tty_put(tty);  // Släpp tty
        return strlen(data_buffer);  // Returnera storleken på den lästa datan
    } else {
        pr_err("TTY device not found\n");
        return -ENODEV;  // Om tty inte hittas, returnera fel
    }
}

static ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    struct tty_struct *tty = NULL;

    // Hämta tty för den aktuella UART-porten (här använd UART0 som exempel)
    tty = tty_get_by_name("ttyAMA0");

    if (count > sizeof(data_buffer) - 1) {
        tty_put(tty);  // Släpp tty
        return -EINVAL;  // Om data är för stor, returnera fel
    }
    if (copy_from_user(data_buffer, buf, count)) {
        tty_put(tty);  // Släpp tty vid fel
        return -EFAULT;  // Om kopiering misslyckas, returnera fel
    }
    data_buffer[count] = '\0';  // Null-terminera strängen

    if (tty) {
        uart_send_data(tty, data_buffer);  // Skicka data till UART
        tty_put(tty);  // Släpp tty
    }

    return count;  // Returnera antal bytes som skrivits
}

// File operations struktur för /proc-filhantering
static struct file_operations fops = {
    .read = proc_read,
    .write = proc_write,
};

// Modul init (modulens startpunkt)
static int __init sram_uart_module_init(void)
{
    // Skapa en PROC-fil som gör det möjligt att läsa och skriva data till och från modulen
    proc_entry = proc_create(DEVICE_NAME, 0666, NULL, &fops);
    if (!proc_entry) {
        pr_err("Failed to create /proc entry\n");
        return -ENOMEM;  // Om vi misslyckas att skapa PROC-filen, returnera fel
    }

    pr_info("SRAM UART Module Initialized\n");
    return 0;  // Returnera 0 för att indikera att modulen har laddats
}

// Modul exit (modulens avslutning)
static void __exit sram_uart_module_exit(void)
{
    // Rensa PROC-filen
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
