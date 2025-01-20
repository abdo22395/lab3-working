#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/fs.h>
#include <linux/termios.h>
#include <linux/delay.h>

#define PROC_NAME "my_module"
#define BUFFER_SIZE 256
#define UART_DEVICE "/dev/ttyAMA0"  // Raspberry Pi default UART device (might be ttyS0, check your system)

// Variables
static struct proc_dir_entry *proc_entry;
static char buffer[BUFFER_SIZE];
static struct file *uart_file = NULL;

// Function to generate random heart rate value (between 60 and 120)
int generate_random_heart_rate(void) {
    return (prandom_u32() % 61) + 60;  // Random number between 60 and 120
}

// Function to configure UART (9600 baud, 8N1)
int configure_uart(struct file *uart_file) {
    struct termios options;

    // Get the current settings
    if (tcgetattr(uart_file->f_inode->i_rdev, &options) < 0) {
        printk(KERN_ERR "Failed to get UART attributes\n");
        return -EINVAL;
    }

    // Set baud rate to 9600
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    // 8N1 (8 data bits, no parity, 1 stop bit)
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;     // 8 data bits

    // No hardware flow control
    options.c_cflag &= ~CRTSCTS;

    // Enable receiver and set local mode
    options.c_cflag |= (CLOCAL | CREAD);

    // No software flow control
    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Set the new attributes
    if (tcsetattr(uart_file->f_inode->i_rdev, TCSANOW, &options) < 0) {
        printk(KERN_ERR "Failed to set UART attributes\n");
        return -EINVAL;
    }

    return 0;
}

// Function to send data to UART
int send_data_to_uart(int heart_rate) {
    char send_buffer[256];
    int bytes_written;

    // Prepare the message to send
    snprintf(send_buffer, sizeof(send_buffer), "Heart rate: %d bpm\n", heart_rate);

    // Write the message to UART
    bytes_written = vfs_write(uart_file, send_buffer, strlen(send_buffer), &uart_file->f_pos);
    if (bytes_written < 0) {
        printk(KERN_ERR "Failed to write to UART\n");
        return -EIO;
    }

    printk(KERN_INFO "Sent to UART: %s", send_buffer);
    return bytes_written;
}

// Read function for /proc/my_module
static ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos) {
    int heart_rate = generate_random_heart_rate();
    int len = snprintf(buffer, BUFFER_SIZE, "Random heart rate: %d bpm\n", heart_rate);

    // Send the heart rate to UART
    send_data_to_uart(heart_rate);

    // Return the data in the buffer to the user
    return simple_read_from_buffer(usr_buf, count, pos, buffer, len);
}

// Define proc_ops structure
static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

// Module initialization
static int __init hello_init(void) {
    // Open UART device file (ttyAMA0 is commonly used on Raspberry Pi)
    uart_file = filp_open(UART_DEVICE, O_RDWR | O_NOCTTY | O_SYNC, 0);
    if (IS_ERR(uart_file)) {
        printk(KERN_ERR "Failed to open UART device\n");
        return PTR_ERR(uart_file);
    }

    // Configure the UART settings
    if (configure_uart(uart_file) != 0) {
        filp_close(uart_file, NULL);
        return -EINVAL;
    }

    // Create /proc entry for reading heart rate
    proc_entry = proc_create(PROC_NAME, 0, NULL, &proc_fops);
    if (proc_entry == NULL) {
        printk(KERN_ALERT "Failed to create /proc/%s\n", PROC_NAME);
        filp_close(uart_file, NULL);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
    return 0;
}

// Module cleanup
static void __exit hello_exit(void) {
    proc_remove(proc_entry);
    filp_close(uart_file, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module generating random heart rate values and sending over UART");

module_init(hello_init);
module_exit(hello_exit);
