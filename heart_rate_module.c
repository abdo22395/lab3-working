#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // File control definitions
#include <termios.h>    // POSIX terminal control definitions
#include <unistd.h>     // UNIX standard function definitions
#include <errno.h>
#include <time.h>       // For random number generation

// Function to configure UART
int configure_uart(int fd) {
    struct termios options;

    // Read current settings
    if (tcgetattr(fd, &options) < 0) {
        perror("tcgetattr");
        return -1;
    }

    // Set baud rate
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    // 8N1 (8 data bits, no parity, 1 stop bit)
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;     // 8 data bits

    // No hardware flow control
    options.c_cflag &= ~CRTSCTS;

    // Enable receiver and set local mode
    options.c_cflag |= (CLOCAL | CREAD);

    // No software flow control
    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Raw mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Raw output mode
    options.c_oflag &= ~OPOST;

    // Timeout and minimum number of characters
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10; // Timeout in deciseconds

    // Set the new settings
    if (tcsetattr(fd, TCSANOW, &options) < 0) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

// Function to generate random heart rate values (between 60 and 120)
int generate_random_heart_rate() {
    return (rand() % 61) + 60;  // Random number between 60 and 120
}

int main() {
    int uart_fd;
    char *uart_device = "/dev/ttyACM0"; // Change this if needed
    char send_buffer[256];
    char recv_buffer[256];
    int bytes_written, bytes_read;

    // Seed random number generator
    srand(time(NULL));

    // Open the UART device
    uart_fd = open(uart_device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd == -1) {
        perror("open UART");
        return EXIT_FAILURE;
    }

    // Configure the UART
    if (configure_uart(uart_fd) != 0) {
        close(uart_fd);
        return EXIT_FAILURE;
    }

    // Make the UART device non-blocking
    fcntl(uart_fd, F_SETFL, 0);

    // Generate a random heart rate value
    int heart_rate = generate_random_heart_rate(); // Random value between 60 and 120
    snprintf(send_buffer, sizeof(send_buffer), "Heart rate: %d bpm\n", heart_rate);

    // Send random heart rate to Arduino via UART
    bytes_written = write(uart_fd, send_buffer, strlen(send_buffer));
    if (bytes_written < 0) {
        perror("write");
        close(uart_fd);
        return EXIT_FAILURE;
    }
    printf("Sent %d bytes: %s", bytes_written, send_buffer);

    // Wait for a response from the Arduino
    memset(recv_buffer, 0, sizeof(recv_buffer));
    bytes_read = read(uart_fd, recv_buffer, sizeof(recv_buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(uart_fd);
        return EXIT_FAILURE;
    } else if (bytes_read == 0) {
        printf("No data received.\n");
    } else {
        recv_buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Received %d bytes: %s", bytes_read, recv_buffer);
    }

    // Close the UART device
    close(uart_fd);

    return EXIT_SUCCESS;
}
