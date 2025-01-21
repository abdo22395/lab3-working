#include <SPI.h> // SPI communication library
#include <C:\Users\fafa2\OneDrive\Desktop\Lab3\arduino\NVSRAM.h>  // Include the NVSRAM library from GitHub

// Define the pin used for chip select (CS) on the microcontroller
#define CHIP_SELECT_PIN 10 // Pin 10 is used to select the SRAM chip for communication

// Create an instance of the NVSRAM class to interact with the SRAM chip
NVSRAM nvSRAM(CHIP_SELECT_PIN); // Initialize NVSRAM with the chip select pin

void setup() {
  // Start serial communication at 9600 baud rate for debugging
  Serial.begin(9600);

  // Initialize the NVSRAM chip
  nvSRAM.begin(); 
}

void loop() {
  // Wait for a command from the serial monitor and store it in a String variable
  String command = Serial.readStringUntil('\n'); // Read the serial input until a newline is received
  
  // Trim any leading or trailing whitespace from the command
  command.trim();

  // If the command starts with "WRITE", perform a write operation
  if (command.startsWith("WRITE")) {
    // Extract the address and data to write (after the "WRITE" keyword)
    int spaceIndex = command.indexOf(' '); // Find the first space after the "WRITE" command
    if (spaceIndex != -1) {
      String addressString = command.substring(6, spaceIndex); // Extract address
      String dataString = command.substring(spaceIndex + 1); // Extract data after the space

      int address = addressString.toInt(); // Convert address to an integer
      // Determine if the data is a number or text
      if (dataString.length() > 0 && isDigit(dataString[0])) {
        int data = dataString.toInt(); // If it's a number, convert to integer
        nvSRAM.write(address, data); // Write the integer value to NVSRAM at the given address
        Serial.print("Written number: ");
        Serial.println(data);
      } else {
        // If it's not a number, treat it as text
        char data[dataString.length() + 1];
        dataString.toCharArray(data, dataString.length() + 1);
        for (int i = 0; i < dataString.length(); i++) {
          nvSRAM.write(address + i, data[i]); // Write each character to the NVSRAM
        }
        Serial.print("Written text: ");
        Serial.println(dataString);
      }
    } else {
      Serial.println("Error: Incorrect WRITE command format. Use WRITE <address> <data>");
    }
  }
  // If the command starts with "READ", perform a read operation
  else if (command.startsWith("READ")) {
    // Extract the address to read from (after the "READ" keyword)
    String addressString = command.substring(5);
    int address = addressString.toInt(); // Convert address to an integer

    // Read the data at the given address
    String result = "";
    char readData;
    int i = 0;
    while (true) {
      readData = nvSRAM.read(address + i); // Read byte by byte from NVSRAM
      if (readData == 0) break; // Stop if we encounter a null byte (end of string)
      result += readData; // Append the byte to the result string
      i++;
    }

    // Print the data read from NVSRAM
    Serial.print("Read data: ");
    Serial.println(result); // Output the data value to the serial monitor
  }
  else {
    Serial.println("Error: Invalid command. Use WRITE <address> <data> or READ <address>");
  }
}