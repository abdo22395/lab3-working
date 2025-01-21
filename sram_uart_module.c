#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

#define PROCFS_BUFFER_SIZE  256  // Buffer size to accommodate larger strings or numbers
static char proc_buffer[PROCFS_BUFFER_SIZE];
static unsigned long buffer_size = 0;

// Function to handle read operations from the /proc file
static ssize_t read_proc_file(struct file *file, char __user *user_buffer,
                               size_t count, loff_t *offset)
{
    printk(KERN_INFO "Read operation initiated\n");

    // If we have already read the file or if the buffer is too small, return 0 (EOF)
    if (*offset > 0 || count < PROCFS_BUFFER_SIZE) {
        return 0;
    }

    // For demonstration purposes, we’ll simulate a read from SRAM.
    // In a real scenario, you'd read from SRAM here.
    // Assuming `proc_buffer` contains data like text or numbers to return.

    // Check if the buffer contains text or numbers
    if (isalpha(proc_buffer[0])) {
        // If the first byte is a letter, treat it as text
        printk(KERN_INFO "Returning text data: %s\n", proc_buffer);
    } else {
        // If the first byte is a digit, treat it as a number
        printk(KERN_INFO "Returning number data: %s\n", proc_buffer);
    }

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

    // Print the received data to the kernel log
    printk(KERN_INFO "Received data: %s\n", proc_buffer);

    // In this case, we don't process the data further, but you could write it to /dev/ttyACM0
    // or perform other actions depending on your requirements.

    // Here we simply echo the data back as a string
    // If it's a number, handle it accordingly (this can be extended if needed)
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
    proc_create("custom_output", 0666, NULL, &proc_file_operations);
    pr_info("Module 'custom_output' is being loaded!\n");
    return 0;
}

// Module exit function
static void __exit module_exit_function(void) {
    // Remove the /proc file entry
    proc_remove(proc_entry);
    pr_info("Module 'custom_output' has been unloaded\n");
}

// Register the module initialization and exit functions
module_init(module_init_function);
module_exit(module_exit_function);

// License and description for the module
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Custom Kernel Module for Serial Communication with SRAM");
