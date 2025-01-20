#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

#define DEVICE_NAME "heart_rate_device"
#define PROC_ENTRY_NAME "heart_rate_data"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel Module to Handle Heart Rate Data");

static struct proc_dir_entry *heart_rate_proc_file;

// Function to call Python script for saving heart rate data to MariaDB
static int write_heart_rate_to_db(const char *heart_rate)
{
    char command[256];
    snprintf(command, sizeof(command), "python3 /path/to/your/insert_heart_rate.py %s", heart_rate);
    
    int ret = system(command);  // Executes the Python script
    if (ret == -1) {
        pr_err("Error executing the system command\n");
    }
    
    return ret;
}

// Function to handle writing data to the /proc/ entry
static ssize_t proc_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    char heart_rate[100];

    // Read data from the user space (heart rate value)
    if (copy_from_user(heart_rate, buf, len)) {
        return -EFAULT;
    }

    heart_rate[len] = '\0';  // Null-terminate the string

    pr_info("Received heart rate: %s\n", heart_rate);

    // Call Python script to store heart rate in the database
    write_heart_rate_to_db(heart_rate);

    return len;  // Return number of bytes written
}

// File operations structure
static const struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .write = proc_write,
};

// Module initialization function
static int __init heart_rate_init(void)
{
    pr_info("Heart rate kernel module initialized.\n");

    // Create /proc entry to receive heart rate data
    heart_rate_proc_file = proc_create(PROC_ENTRY_NAME, 0666, NULL, &proc_fops);
    if (!heart_rate_proc_file) {
        pr_err("Failed to create /proc entry\n");
        return -ENOMEM;
    }

    return 0;  // Success
}

// Module exit function
static void __exit heart_rate_exit(void)
{
    pr_info("Heart rate kernel module exiting.\n");

    // Remove the /proc entry
    remove_proc_entry(PROC_ENTRY_NAME, NULL);
}

module_init(heart_rate_init);
module_exit(heart_rate_exit);
