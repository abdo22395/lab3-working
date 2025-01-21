#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define PROC_PATH "/proc/custom_output"

// Function to generate a random string of a given length
void generate_random_string(char *str, size_t len) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < len; i++) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    str[len] = '\0'; // Null terminate the string
}

int main() {
    char random_data[20];
    char read_buffer[20];
    FILE *proc_file;

    // Seed the random number generator
    srand(time(NULL));

    while (1) {
        // Generate random data to send to the kernel
        generate_random_string(random_data, sizeof(random_data) - 1);
        printf("Sending data to kernel: %s\n", random_data);

        // Open /proc/arduinouno for writing
        proc_file = fopen(PROC_PATH, "w");
        if (proc_file == NULL) {
            perror("Failed to open /proc/arduinouno for writing");
            return 1;
        }

        // Write random data to /proc/arduinouno
        fprintf(proc_file, "%s", random_data);
        fclose(proc_file);

        // Wait a bit before reading
        sleep(1);

        // Open /proc/arduinouno for reading
        proc_file = fopen(PROC_PATH, "r");
        if (proc_file == NULL) {
            perror("Failed to open /proc/arduinouno for reading");
            return 1;
        }

        // Read data from /proc/arduinouno (this data comes from UART)
        size_t bytes_read = fread(read_buffer, 1, sizeof(read_buffer) - 1, proc_file);
        read_buffer[bytes_read] = '\0'; // Null terminate the string

        // Print the data we read from UART
        if (bytes_read > 0) {
            printf("Read data from UART: %s\n", read_buffer);
        } else {
            printf("No data read from UART.\n");
        }

        fclose(proc_file);

        // Wait before sending the next random data
        sleep(2);
    }

    return 0;
}
