#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define PROC_PATH "/proc/custom_output"

// Function to generate a random number as a string
void generate_random_number(char *str, size_t len) {
    int random_num = rand() % 1000000;  // Generate a random number between 0 and 999999
    snprintf(str, len, "%d", random_num);  // Convert the number to a string
}

int main() {
    char random_data[20];
    char read_buffer[20];
    FILE *proc_file;

    // Seed the random number generator
    srand(time(NULL));

    while (1) {
        // Generate random number to send to the kernel
        generate_random_number(random_data, sizeof(random_data) - 1);
        printf("Sending data to kernel: %s\n", random_data);

        // Open /proc/custom_output for writing
        proc_file = fopen(PROC_PATH, "w");
        if (proc_file == NULL) {
            perror("Failed to open /proc/custom_output for writing");
            return 1;
        }

        // Write random number to /proc/custom_output
        fprintf(proc_file, "%s", random_data);
        fclose(proc_file);

        // Wait a bit before reading
        sleep(1);

        // Open /proc/custom_output for reading
        proc_file = fopen(PROC_PATH, "r");
        if (proc_file == NULL) {
            perror("Failed to open /proc/custom_output for reading");
            return 1;
        }

        // Read data from /proc/custom_output (this data comes from UART)
        size_t bytes_read = fread(read_buffer, 1, sizeof(read_buffer) - 1, proc_file);
        read_buffer[bytes_read] = '\0'; // Null terminate the string

        // Print the data we read from UART
        if (bytes_read > 0) {
            printf("Read data from UART: %s\n", read_buffer);
        } else {
            printf("No data read from UART.\n");
        }

        fclose(proc_file);

        // Wait before sending the next random number
        sleep(2);
    }

    return 0;
}
