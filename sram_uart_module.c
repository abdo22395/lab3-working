#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/termios.h>
#include <linux/tty.h>
#include <linux/fcntl.h>
#include <linux/errno.h>

#define BUFFER_SIZE 5
#define SERIAL_DEVICE "/dev/ttyACM0"
#define PROCFS_MAX_LENGTH 20

static struct proc_dir_entry* proc_file;
static char proc_buffer[PROCFS_MAX_LENGTH];
static unsigned long proc_buffer_size = 0;

static ssize_t proc_read(struct file* file, char __user* user_buffer, size_t count, loff_t* offset)
{
    struct file *serial_device_file;
    loff_t serial_offset = 0;
    ssize_t bytes_read;
    char temp_buffer[PROCFS_MAX_LENGTH] = {0};

    printk(KERN_INFO "Invoked proc_read\n");
    if (*offset > 0 || count < PROCFS_MAX_LENGTH){
        return 0;
    }

    // Open the serial device for communication
    serial_device_file = filp_open(SERIAL_DEVICE, O_RDWR, 0);
    ssleep(1); 
    if(IS_ERR(serial_device_file))
    {
        printk(KERN_ERR "Failed to open serial device\n");
        return PTR_ERR(serial_device_file);
    }

    // Read data from the serial device into temp_buffer
    bytes_read = kernel_read(serial_device_file, &temp_buffer, PROCFS_MAX_LENGTH, NULL);
    ssleep(1);

    printk("%ld", bytes_read);
    // Close the serial device connection
    filp_close(serial_device_file, NULL);
    
    proc_buffer_size = bytes_read;
   
    // Check if reading was successful
    if(bytes_read < 0)
    {
        printk(KERN_INFO "Error reading from serial device\n");
        return bytes_read;
    }

    printk(KERN_INFO "Received from serial device: %s\n" , proc_buffer);

    int ret = copy_to_user(user_buffer, proc_buffer, proc_buffer_size);
    if(ret != 0)
    {
        printk(KERN_ERR "Failed to copy data to user space\n");
        return -EFAULT;
    }
    printk(KERN_INFO "Data copied to user space\n");

    // Clear the buffer
    memset(proc_buffer, 0, sizeof(proc_buffer));

    // Update the offset and return the number of bytes
    if(*offset > 0)
    {
        return 0;
    }

    *offset = proc_buffer_size;
    return proc_buffer_size;
}   

static ssize_t proc_write(struct file* file, const char __user* user_buffer, size_t count, loff_t* offset)
{
    struct file *serial_device_file;
    char *kernel_buffer; 
    ssize_t bytes_written;
    loff_t serial_offset = 0;
    int kernel_buffer_length;

    printk(KERN_INFO "Invoked proc_write\n");

    // Check if the write size exceeds the allowed limit
    if(count > sizeof(proc_buffer) - 1)
    {
        printk(KERN_ERR "Write size is too large\n");
        return -EINVAL; 
    }

    kernel_buffer = kzalloc((count + 1), GFP_KERNEL);
    if(!kernel_buffer)
    {
        return -ENOMEM;
    }

    if(copy_from_user(kernel_buffer, user_buffer, count))
    {
        kfree(kernel_buffer);
        return -EFAULT;
    }
    kernel_buffer[count] = '\0';
    printk("Write count is: %ld", count);

    // Copy the data into the proc_buffer
    strncpy(proc_buffer, kernel_buffer, count);
    proc_buffer_size = count;

    // Open the serial device for communication
    serial_device_file = filp_open(SERIAL_DEVICE , O_RDWR, 0);
    ssleep(1);
    if(IS_ERR(serial_device_file))
    {
        printk(KERN_ERR "Failed to open serial device\n");
        kfree(kernel_buffer);
        return PTR_ERR(serial_device_file);
    }

    // Write data to the serial device
    bytes_written = kernel_write(serial_device_file, kernel_buffer, count , NULL);
    ssleep(1);
    filp_close(serial_device_file, NULL);
    
    kernel_buffer_length = PROCFS_MAX_LENGTH;
    if(count > PROCFS_MAX_LENGTH)
    {
        kernel_buffer_length = count;
    }

    // Clear the buffer
    memcpy(&proc_buffer, kernel_buffer, kernel_buffer_length);
    printk(KERN_INFO "Writing to serial device: %s\n", proc_buffer);

    // Check if the correct number of bytes were written
    if(bytes_written < 0)
    {
        printk(KERN_ERR "Error writing to serial device\n");
        return bytes_written;
    }
    if(bytes_written != count)
    {
        printk(KERN_ERR "Only partial data written to serial device\n");
        return -EIO;
    }
    proc_buffer_size = kernel_buffer_length;
    kfree(kernel_buffer); 

    *offset = bytes_written;
    return kernel_buffer_length;
}

static const struct proc_ops proc_file_operations = {
    .proc_read = proc_read, 
    .proc_write = proc_write,
};

static int __init module_init_func(void) {
    proc_file = proc_create("arduinouno", 0666, NULL, &proc_file_operations);
    if(!proc_file)
    {
        printk(KERN_ERR "Failed to create /proc/arduinouno\n");
        return -ENOMEM;
    }
    pr_info("Module 'arduinouno' loaded successfully!\n");
    return 0;
}

static void __exit module_exit_func(void) {
    proc_remove(proc_file);
    pr_info("Kernel module removed successfully\n");
}

module_init(module_init_func);
module_exit(module_exit_func);

MODULE_LICENSE("GPL");
