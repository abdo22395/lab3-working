#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>

#define PROCFS_BUFFER_SIZE  256  // Increased buffer size to handle large strings
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

    // Open the device file /dev/ttyACM0 for reading
    struct file* device_file = filp_open("/dev/ttyACM0", O_RDONLY, 0);
    if (IS_ERR(device_file)) {
        printk(KERN_ERR "Failed to open /dev/ttyACM0: %ld\n", PTR_ERR(device_file));
        return -EFAULT;
    }

    // Read data from /dev/ttyACM0 into a local buffer
    char local_buffer[PROCFS_BUFFER_SIZE] = {0};
    ssize_t bytes_read = kernel_read(device_file, local_buffer, PROCFS_BUFFER_SIZE, 0);
    filp_close(device_file, NULL);

    if (bytes_read < 0) {
        printk(KERN_ERR "Error reading from /dev/ttyACM0: %zd\n", bytes_read);
        return -EFAULT;
    }

    // Copy the data we read from the local buffer to the user space buffer
    if (copy_to_user(user_buffer, local_buffer, bytes_read)) {
        return -EFAULT;
    }

    *offset = bytes_read;

    // Return the number of bytes successfully copied to user space
    return bytes_read;
}

// Function to handle write operations to the /proc file
static ssize_t write_proc_file(struct file* file, const char __user* user_buffer,
                                size_t count, loff_t* offset)
{
    printk(KERN_INFO "Write operation initiated\n");

    // Write the string "HELLO MAN" directly to NVSRAM at address 100
    const char *write_string = "HELLO MAN";
    int address = 100;

    // Write each character of "HELLO MAN" to NVSRAM at the specified address
    for (int i = 0; i < strlen(write_string); i++) {
        nvSRAM.write(address + i, write_string[i]); // Write each byte to NVSRAM
    }

    printk(KERN_INFO "Written data to NVSRAM: %s\n", write_string);

    // Simulate reading the data back into proc_buffer
    snprintf(proc_buffer, PROCFS_BUFFER_SIZE, "%s", write_string);
    buffer_size = strlen(proc_buffer);

    return count;
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

    // Write "HELLO MAN" to NVSRAM at address 100 as soon as the module is loaded
    write_proc_file(NULL, NULL, 0, NULL);

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
MODULE_DESCRIPTION("Custom Kernel Module for Serial Communication");
