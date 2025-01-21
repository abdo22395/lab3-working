#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/delay.h>  // For msleep()

#define PROCFS_BUFFER_SIZE  256  // Increased buffer size to handle more data
static char proc_buffer[PROCFS_BUFFER_SIZE];
static unsigned long buffer_size = 0;
static bool written_once = false;  // Flag to track if data has been written to UART

// Declare the proc file entry
static struct proc_dir_entry *proc_file_entry;

// Function to handle read operations from the /proc file
static ssize_t read_proc_file(struct file *file, char __user *user_buffer,
                               size_t count, loff_t *offset)
{
    printk(KERN_INFO "Read operation initiated\n");

    // Return 0 (EOF) if we have already read the file or if the buffer is too small
    if (*offset > 0 || count < PROCFS_BUFFER_SIZE) {
        return 0;
    }

    // If we have not written to UART yet, do it once
    if (!written_once) {
        // Generate a random string
        char random_string[32];
        get_random_bytes(random_string, sizeof(random_string));

        // Format the command to be written to the UART
        char uart_command[PROCFS_BUFFER_SIZE];
        snprintf(uart_command, sizeof(uart_command), "Write '%s' 100", random_string);

        // Open the device file /dev/ttyACM0 for writing
        struct file* device_file = filp_open("/dev/ttyACM0", O_WRONLY, 0);
        if (IS_ERR(device_file)) {
            printk(KERN_ERR "Failed to open /dev/ttyACM0: %ld\n", PTR_ERR(device_file));
            return -EFAULT;
        }

        ssize_t bytes_written = kernel_write(device_file, uart_command, strlen(uart_command), offset);
        filp_close(device_file, NULL);

        if (bytes_written < 0) {
            printk(KERN_ERR "Error writing to /dev/ttyACM0: %zd\n", bytes_written);
            return -EFAULT;
        }

        printk(KERN_INFO "Sent to UART: %s\n", uart_command);

        // Add a delay (e.g., 200ms) before sending the READ command
        msleep(200); // 200 ms delay

        // Now send the READ command
        char read_command[] = "READ 100";
        device_file = filp_open("/dev/ttyACM0", O_WRONLY, 0);
        if (IS_ERR(device_file)) {
            printk(KERN_ERR "Failed to open /dev/ttyACM0 for READ command: %ld\n", PTR_ERR(device_file));
            return -EFAULT;
        }

        bytes_written = kernel_write(device_file, read_command, strlen(read_command), offset);
        filp_close(device_file, NULL);

        if (bytes_written < 0) {
            printk(KERN_ERR "Error writing READ command to /dev/ttyACM0: %zd\n", bytes_written);
            return -EFAULT;
        }

        printk(KERN_INFO "Sent READ command to UART: %s\n", read_command);

        // Set the flag to prevent further writes
        written_once = false;
    }

    // Add a delay (e.g., 100ms) before reading the UART response
    msleep(100); // 100 ms delay

    // Now listen on the UART and capture the response
    struct file* uart_device_file = filp_open("/dev/ttyACM0", O_RDONLY, 0);
    if (IS_ERR(uart_device_file)) {
        printk(KERN_ERR "Failed to open /dev/ttyACM0 for reading: %ld\n", PTR_ERR(uart_device_file));
        return -EFAULT;
    }

    char uart_response[PROCFS_BUFFER_SIZE] = {0};
    ssize_t bytes_read = kernel_read(uart_device_file, uart_response, sizeof(uart_response), 0);
    filp_close(uart_device_file, NULL);

    if (bytes_read < 0) {
        printk(KERN_ERR "Error reading from /dev/ttyACM0: %zd\n", bytes_read);
        return -EFAULT;
    }

    // Add a delay after reading the response (e.g., 200ms)
    msleep(200); // 200 ms delay

    // Copy the UART response to the proc buffer to send to user space
    snprintf(proc_buffer, PROCFS_BUFFER_SIZE, "Received from UART: %s", uart_response);
    buffer_size = strlen(proc_buffer);

    // Return the response to user space
    return buffer_size;
}

// Function to handle write operations to the /proc file (not needed for this use case)
static ssize_t write_proc_file(struct file* file, const char __user* user_buffer,
                                size_t count, loff_t* offset)
{
    printk(KERN_INFO "Write operation initiated\n");

    if (count > PROCFS_BUFFER_SIZE) {
        return -EINVAL;
    }

    // Copy data from user space into the proc_buffer (just for logging)
    if (copy_from_user(proc_buffer, user_buffer, count)) {
        return -EFAULT;
    }

    proc_buffer[count] = '\0';  // Null-terminate the string
    buffer_size = count;        // Update the buffer size

    printk(KERN_INFO "Received data: %s\n", proc_buffer);

    return count;  // No further action on write for this use case
}

// Define the file operations for the /proc file
static const struct proc_ops proc_file_operations = {
  .proc_read = read_proc_file,
  .proc_write = write_proc_file,
};

// Module initialization function
static int __init module_init_function(void) {
    proc_file_entry = proc_create("custom_output", 0666, NULL, &proc_file_operations);
    if (!proc_file_entry) {
        printk(KERN_ERR "Failed to create /proc/custom_output\n");
        return -ENOMEM;
    }

    pr_info("Module 'custom_output' is being loaded!\n");
    return 0;
}

// Module exit function
static void __exit module_exit_function(void) {
    proc_remove(proc_file_entry);
    pr_info("Module 'custom_output' has been unloaded\n");
}

module_init(module_init_function);
module_exit(module_exit_function);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Custom Kernel Module for UART Communication with Delays");
