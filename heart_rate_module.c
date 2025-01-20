#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/termios.h>
#include <linux/random.h>
#include <linux/proc_fs.h>

#define PROC_ENTRY_NAME "heart_rate_data"
#define UART_DEV_PATH "/dev/ttyUSB0"  // Change to your UART device path
#define HEART_RATE_MIN 60  // Min heart rate
#define HEART_RATE_MAX 120  // Max heart rate

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel Module to Generate Heart Rate Data and Send to Arduino via UART");

static struct proc_dir_entry *heart_rate_proc_file;
static struct tty_struct *uart_tty;

static ssize_t proc_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    // This function isn't used in this case because we generate heart rate data internally.
    return 0;  // Nothing to do here
}

static const struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .write = proc_write,  // No writing is necessary for now
};

// Function to send data to Arduino via UART
static int send_to_uart(const char *heart_rate)
{
    int ret;

    if (!uart_tty) {
        uart_tty = tty_open(UART_DEV_PATH, NULL);
        if (IS_ERR(uart_tty)) {
            pr_err("Failed to open UART device\n");
            return PTR_ERR(uart_tty);
        }
    }

    // Write heart rate data to UART
    ret = tty_write(uart_tty, heart_rate, strlen(heart_rate));
    if (ret < 0) {
        pr_err("Failed to write to UART\n");
    }

    pr_info("Sent heart rate: %s to UART\n", heart_rate);

    return ret;
}

// Function to generate random heart rate values
static void generate_random_heart_rate(char *heart_rate_str)
{
    int random_heart_rate;

    // Generate random heart rate between HEART_RATE_MIN and HEART_RATE_MAX
    get_random_bytes(&random_heart_rate, sizeof(random_heart_rate));
    random_heart_rate = HEART_RATE_MIN + (random_heart_rate % (HEART_RATE_MAX - HEART_RATE_MIN));

    // Convert random heart rate to a string
    snprintf(heart_rate_str, 10, "%d", random_heart_rate);
}

// Kernel thread to periodically generate heart rate and send to UART
static int heart_rate_thread(void *data)
{
    char heart_rate[10];
    while (!kthread_should_stop()) {
        generate_random_heart_rate(heart_rate);  // Generate a random heart rate

        // Send the generated heart rate to the Arduino via UART
        send_to_uart(heart_rate);

        // Sleep for 1 second
        ssleep(1);
    }

    return 0;
}

static struct task_struct *heart_rate_task;

// Kernel module initialization
static int __init heart_rate_init(void)
{
    pr_info("Heart rate kernel module initialized.\n");

    // Create /proc entry for heart rate data (though we don't use it in this case)
    heart_rate_proc_file = proc_create(PROC_ENTRY_NAME, 0666, NULL, &proc_fops);
    if (!heart_rate_proc_file) {
        pr_err("Failed to create /proc entry\n");
        return -ENOMEM;
    }

    // Create a kernel thread to generate and send heart rate data
    heart_rate_task = kthread_run(heart_rate_thread, NULL, "heart_rate_thread");
    if (IS_ERR(heart_rate_task)) {
        pr_err("Failed to create heart rate thread\n");
        remove_proc_entry(PROC_ENTRY_NAME, NULL);
        return PTR_ERR(heart_rate_task);
    }

    return 0;  // Success
}

// Kernel module exit
static void __exit heart_rate_exit(void)
{
    pr_info("Heart rate kernel module exiting.\n");

    // Stop the heart rate generation thread
    if (heart_rate_task) {
        kthread_stop(heart_rate_task);
    }

    // Remove the /proc entry
    remove_proc_entry(PROC_ENTRY_NAME, NULL);

    if (uart_tty) {
        tty_release(uart_tty);
    }
}

module_init(heart_rate_init);
module_exit(heart_rate_exit);
