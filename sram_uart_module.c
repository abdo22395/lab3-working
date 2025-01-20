#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/errno.h>

#define MESSAGE_LENGTH 5
#define PROCFS_MAX_SIZE 256
#define UART_DEVICE "/dev/ttyACM1"  // Change this to your specific UART device
#define UART_BAUDRATE 9600  // Adjust as necessary

static struct proc_dir_entry* proc_entry;
static char read_proc_buffer[PROCFS_MAX_SIZE];
static unsigned long read_proc_buffer_size = 0;

// Function to send data to UART (Arduino)
static void uart_send_data(const char *data)
{
    struct file *testfd;
    ssize_t bytes_written;

    testfd = filp_open(UART_DEVICE, O_RDWR, 0);
    if (IS_ERR(testfd)) {
        pr_err("Failed to open UART device: %ld\n", PTR_ERR(testfd));
        return;
    }

    bytes_written = kernel_write(testfd, data, strlen(data), &testfd->f_pos);
    if (bytes_written < 0) {
        pr_err("Failed to write to UART device\n");
    }

    filp_close(testfd, NULL);
}

// Function to read data from UART (Arduino)
static ssize_t uart_read_data(void)
{
    struct file *testfd;
    ssize_t bytes_read;
    mm_segment_t old_fs;

    testfd = filp_open(UART_DEVICE, O_RDONLY, 0);
    if (IS_ERR(testfd)) {
        pr_err("Failed to open UART device: %ld\n", PTR_ERR(testfd));
        return -EINVAL;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);  // Allow reading from user space addresses

    bytes_read = kernel_read(testfd, read_proc_buffer, PROCFS_MAX_SIZE, &testfd->f_pos);
    if (bytes_read < 0) {
        pr_err("Failed to read from UART device\n");
        set_fs(old_fs);  // Restore original address space
        filp_close(testfd, NULL);
        return -EINVAL;
    }

    set_fs(old_fs);  // Restore original address space
    read_proc_buffer_size = bytes_read;
    filp_close(testfd, NULL);

    return bytes_read;
}

static ssize_t read_proc(struct file* file, char __user* user_buffer, size_t count, loff_t* offset)
{
    printk(KERN_INFO "Called read_proc\n");

    if (*offset > 0 || count < PROCFS_MAX_SIZE) {
        return 0;  // No more data to read
    }

    // Read data from the Arduino via UART
    ssize_t bytes_read = uart_read_data();
    if (bytes_read < 0) {
        return bytes_read;  // Error during UART read
    }

    // Copy data to user space
    if (copy_to_user(user_buffer, read_proc_buffer, read_proc_buffer_size)) {
        return -EFAULT;
    }

    *offset = read_proc_buffer_size;
    return read_proc_buffer_size;
}

static ssize_t write_proc(struct file* file, const char __user* user_buffer, size_t count, loff_t* offset)
{
    printk(KERN_INFO "Called write_proc\n");

    int tmp_len;
    char *tmp = kzalloc(count + 1, GFP_KERNEL);

    if (!tmp) {
        return -ENOMEM;
    }

    // Copy data from user space
    if (copy_from_user(tmp, user_buffer, count)) {
        kfree(tmp);
        return -EFAULT;
    }

    tmp_len = (count > PROCFS_MAX_SIZE) ? PROCFS_MAX_SIZE : count;
    memcpy(&read_proc_buffer, tmp, tmp_len);
    pr_info("write_proc: %s\n", read_proc_buffer);

    // Send data to Arduino via UART
    uart_send_data(read_proc_buffer);

    read_proc_buffer_size = tmp_len;
    kfree(tmp);
    return tmp_len;
}

static const struct proc_ops hello_proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

static int __init construct(void)
{
    proc_entry = proc_create("arduinouno", 0666, NULL, &hello_proc_fops);
    if (!proc_entry) {
        pr_err("Failed to create /proc/arduinouno\n");
        return -ENOMEM;
    }
    pr_info("Loading 'arduinouno' module!\n");
    return 0;
}

static void __exit destruct(void)
{
    proc_remove(proc_entry);
    pr_info("First kernel module has been removed\n");
}

module_init(construct);
module_exit(destruct);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module to interface with Arduino Uno via UART");
