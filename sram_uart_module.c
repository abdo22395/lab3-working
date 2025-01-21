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



// Christoffer Angestam, Mattias Carlsén Friberg


/*

Try reading with 'cat /proc/arduinouno'

Try writing with ' echo 'hello' >> /proc/arduinouno'

*/

#define MESSAGE_LENGTH 5
#define SERIAL_PORT_ARDUINO "/dev/ttyACM0"
#define PROCFS_MAX_SIZE 20

static struct proc_dir_entry* proc_entry;
static char read_proc_buffer[PROCFS_MAX_SIZE];
static unsigned long read_proc_buffer_size = 0;

static ssize_t read_proc(struct file* file, char __user* user_buffer, size_t count, loff_t* offset)
{
    
    // create file, offset and how many bytes have been read.
    struct file *serial_file;
    loff_t serial_offset = 0;
    ssize_t bytes_read;
    char buffer[PROCFS_MAX_SIZE] = {0};

    printk(KERN_INFO "Called read_proc\n");
    if (*offset > 0 || count < PROCFS_MAX_SIZE){
        return 0;
    }

    // open a serial port to communicate with arduino
    serial_file = filp_open(SERIAL_PORT_ARDUINO, O_RDWR, 0);
    ssleep(1); 
    if(IS_ERR(serial_file))
    {
        printk(KERN_ERR "Failed to open serial port\n");
        return PTR_ERR(serial_file);
    }

    // read the file and put it into read_proc_buffer
    bytes_read = kernel_read(serial_file, &buffer, PROCFS_MAX_SIZE, NULL);   //&serial_offset
    ssleep(1);

    printk("%ld", bytes_read);
    // close the communication with the arduino
    filp_close(serial_file, NULL);
    
    read_proc_buffer_size = bytes_read;
   
    // check so that it managed to read
    if(bytes_read < 0)
    {
        printk(KERN_INFO "Error reading from serial port\n");
        return bytes_read;
    }

    
    printk(KERN_INFO "Recived from serial port: %s\n" , read_proc_buffer);

    int ret = copy_to_user(user_buffer, read_proc_buffer, read_proc_buffer_size);
    if(ret != 0)
    {
        printk(KERN_ERR "Failed to copy to user\n");
        return -EFAULT;
    }
    printk(KERN_INFO "copied to user\n");
    // clear the buffer
    memset(read_proc_buffer, 0, sizeof(read_proc_buffer));
    // uppdate the offset and return number of bytes
    if(*offset > 0)
    {
        return 0;
    }

    *offset = read_proc_buffer_size;
    return read_proc_buffer_size;
    
}   

static ssize_t write_proc(struct file* file, const char __user* user_buffer, size_t count, loff_t* offset)
{
    
    struct file *serial_file;
    char *kernel_buffer; // create a buffer to temporarily store the string to be written in.
    ssize_t bytes_written;
    loff_t serial_offset = 0;
    int kern_buff_len;
    printk(KERN_INFO "Called write_proc\n");


    // check so that it doesnt write anything bigger than what is allowed
    if(count > sizeof(read_proc_buffer) - 1)
    {
        printk(KERN_ERR "Writing size to large\n");
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
    printk("count is: %ld", count);

    // copy the read_proc_buffer into the kernle buffer
    strncpy(read_proc_buffer, kernel_buffer, count);
    read_proc_buffer_size = count;

    // print what was written.

    // open the serial to the arduino
    serial_file = filp_open(SERIAL_PORT_ARDUINO , O_RDWR, 0);
    ssleep(1);
    if(IS_ERR(serial_file))
    {
        printk(KERN_ERR "Failed to open serial port\n");
        kfree(kernel_buffer);
        return PTR_ERR(serial_file);
    }

    // write to the arduino
    bytes_written = kernel_write(serial_file, kernel_buffer, count , NULL);
    ssleep(1);
    filp_close(serial_file, NULL);
    
    kern_buff_len = PROCFS_MAX_SIZE;
    if(count > PROCFS_MAX_SIZE)
    {
        kern_buff_len = count;
    }

    // clear the buffer 
    memcpy(&read_proc_buffer, kernel_buffer, kern_buff_len);
    printk(KERN_INFO "Writing to serial port: %s\n", read_proc_buffer);
    // check so that the amount of bytes written checks out with how many should be written.
    if(bytes_written < 0)
    {
        printk(KERN_ERR "Error writing to serial port\n");
        return bytes_written;
    }
    if(bytes_written != count)
    {
        printk(KERN_ERR "Only partial writing to the serial port");
        return -EIO;
    }
    read_proc_buffer_size = kern_buff_len;
    kfree(kernel_buffer); 

    *offset = bytes_written;
    return kern_buff_len;
    

}

static const struct proc_ops hello_proc_fops = {
    .proc_read = read_proc, 
    .proc_write = write_proc,
};


static int __init construct(void) {
    proc_entry = proc_create("arduinouno", 0666, NULL, &hello_proc_fops);
    if(!proc_entry)
    {
        printk(KERN_ERR "Failed to create /proc/arduinouno\n");
        return -ENOMEM;
    }
    pr_info("Loading 'arduinouno' module JAAA!\n");
    return 0;
}

static void __exit destruct(void) {
    proc_remove(proc_entry);
    pr_info("First kernel module has been removed\n");
}

module_init(construct);
module_exit(destruct);

/* Obligatorisk GPL licens för allt relaterat till kerneln, även om vi skapar för eget bruk */
MODULE_LICENSE("GPL");
