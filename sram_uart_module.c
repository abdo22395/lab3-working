#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

#define PROCFS_BUFFER_SIZE  256  // Increased buffer size to handle strings like "hello man"
static char proc_buffer[PROCFS_BUFFER_SIZE];
static unsigned long buffer_size = 0;

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

    // Here, simulate reading the string from /dev/ttyACM0 (or just output our stored data in proc_buffer)
    // You may replace this part with actual code to read from UART (if required)
    // Here we're assuming that we've written the string "hello man" to proc_buffer already

    // Copy the contents of proc_buffer into the user buffer
    if (copy_to_user(user_buffer, proc_buffer, buffer_size)) {
        return -EFAULT;
    }

    // Set the offset so that subsequent reads will return 0 (EOF)
    *offset = buffer_size;

    return buffer_size;
}

// Function to handle write operations to the /proc file
static ssize_t write_proc_file(struct file* file, const char __user* user_buffer,
                                size_t count, loff_t* offset)
{
    printk(KERN_INFO "Write operation initiated\n");

    // Ensure that the data does not exceed the buffer size
    if (count > PROCFS_BUFFER_SIZE) {
        return -EINVAL;
    }

    // Copy data from user space into the proc_buffer
    if (copy_from_user(proc_buffer, user_buffer, count)) {
        return -EFAULT;
    }

    // Null terminate the string to ensure proper handling of text
    proc_buffer[count] = '\0';

    // Update buffer_size with the actual number of bytes written
    buffer_size = count;

    // Print the received data to the kernel log for debugging purposes
    printk(KERN_INFO "Received data: %s\n", proc_buffer);

    // Here, simulate writing the string "hello man" to /dev/ttyACM0
    // If the incoming command is "WRITE", simulate writing "hello man"
    if (strncmp(proc_buffer, "WRITE", 5) == 0) {
        // Assuming the "WRITE" command writes "hello man" to UART
        const char *write_string = "hello man";
        struct file* device_file = filp_open("/dev/ttyACM0", O_RDWR, 0);
        if (IS_ERR(device_file)) {
            printk(KERN_ERR "Failed to open /dev/ttyACM0: %ld\n", PTR_ERR(device_file));
            return -EFAULT;
        }

        // Write "hello man" to /dev/ttyACM0
        ssize_t bytes_written = kernel_write(device_file, write_string, strlen(write_string), offset);
        filp_close(device_file, NULL);

        // Log the written data
        printk(KERN_INFO "Written to /dev/ttyACM0: %s\n", write_string);

        // After writing to UART, simulate the data being read from /dev/ttyACM0
        snprintf(proc_buffer, PROCFS_BUFFER_SIZE, "%s", write_string); // Simulate reading back "hello man"
        buffer_size = strlen(proc_buffer); // Set buffer size to the string length
    } else {
        printk(KERN_INFO "Invalid command, only WRITE is supported.\n");
    }

    return count;
}

// Define the file operations for the /proc file
static const struct proc_ops proc_file_operations = {
  .proc_read = read_proc_file,
  .proc_write = write_proc_file,
};

// Module initialization function
static int __init module_init_function(void) {
    // Create the /proc file entry for custom output
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
    // Remove the /proc file entry
    proc_remove(proc_file_entry);
    pr_info("Module 'custom_output' has been unloaded\n");
}

// Register the module initialization and exit functions
module_init(module_init_function);
module_exit(module_exit_function);

// License and description for the module
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Custom Kernel Module for Serial Communication");
