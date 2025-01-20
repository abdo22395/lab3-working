#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROC_PATH "/proc/arduinouno"

int main() {
    char read_buffer[1024];  // Increased buffer size
    FILE *proc_file;

    while (1) {
        // Open /proc/arduinouno for reading
        proc_file = fopen(PROC_PATH, "r");
        if (proc_file == NULL) {
            perror("Failed to open /proc/arduinouno for reading");
            return 1;
        }

        // Read all available data from the kernel module (until EOF)
        size_t bytes_read = fread(read_buffer, 1, sizeof(read_buffer) - 1, proc_file);
        read_buffer[bytes_read] = '\0';  // Null-terminate the string

        if (bytes_read > 0) {
            printf("Read data from UART: %s\n", read_buffer);
        } else {
            printf("No data read from UART.\n");
        }

        fclose(proc_file);

        // Sleep for a bit before the next read
        sleep(2);
    }

    return 0;
}
