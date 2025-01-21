#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>



#define MAX_MESSAGE_LENGTH 5
static struct proc_dir_entry* proc_file_entry;
#define PROCFS_BUFFER_SIZE     20
static char proc_buffer[PROCFS_BUFFER_SIZE];
static unsigned long buffer_size = 0;

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
    {
        struct file* device_file = filp_open("/dev/ttyACM0", O_RDWR, 0);
        if (IS_ERR(device_file)) {
            printk(KERN_ERR "Failed to open /dev/ttyACM0: %ld\n", PTR_ERR(device_file));
            return -EFAULT;
        }

        // Read data from /dev/ttyACM0 into a local buffer
        {
            char local_buffer[PROCFS_BUFFER_SIZE] = {0};

            // Read up to PROCFS_BUFFER_SIZE bytes. Offset is set to 0 for the serial port.
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

            /*
             * Set the offset to ensure that subsequent reads (e.g., via `cat /proc/custom_output`)
             * will return 0 (EOF).
             */
            *offset = bytes_read;

            // Return the number of bytes successfully copied to user space
            return bytes_read;
        }
    }
}

// Function to handle write operations to the /proc file
static ssize_t write_proc_file(struct file* file, const char __user* user_buffer,
                                size_t count, loff_t* offset)
{
    printk(KERN_INFO "Write operation initiated\n");

    // Allocate temporary buffer for incoming data
    char *temp_buffer = kzalloc(count + 1, GFP_KERNEL);
    if (!temp_buffer)
        return -ENOMEM;

    // Copy data from user space to the kernel buffer
    if (copy_from_user(temp_buffer, user_buffer, count)) {
        kfree(temp_buffer);
        return -EFAULT;
    }

    // Open the device file /dev/ttyACM0 for writing
    struct file* device_file = filp_open("/dev/ttyACM0", O_RDWR, 0);
    ssize_t bytes_written = kernel_write(device_file, temp_buffer, count + 1, offset);

    filp_close(device_file, NULL);

    // Copy the data to the proc buffer for subsequent reads
    size_t write_len = (count > PROCFS_BUFFER_SIZE) ? PROCFS_BUFFER_SIZE : count;
    memcpy(proc_buffer, temp_buffer, write_len);
    buffer_size = write_len;

    // Free the allocated memory for the temporary buffer
    kfree(temp_buffer);

    // Return the number of bytes written
    return write_len;
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
