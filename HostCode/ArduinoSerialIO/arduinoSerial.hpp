#pragma once

#include <stdio.h> 
#include <unistd.h>  
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <array>

/*
    * ArduinoSerial
    * 
    * This class enables communication with an Arduino over a serial port.
    * It is based on the Arduino Serial class, but is not a direct copy.
    * Only works on good operating systems.
    * This version implements timeout functionality.
    * 
    * Features which will not be implemented (in this version):
    * - AvailableForWrite()
    * - Peek()
    * 
    * Notes:
    *  - The configuration of the serial port may not be correct. 
    *  - Flush is set up to behave as it did prior to arduino 1.0, it removes incoming data.
*/

class arduinoSerial {
private:
    int fd; // The file descriptor for the serial port (Usually /dev/ttyACM0)
    std::string ttyName; // The name of the serial port (Usually /dev/ttyACM0). This string exists mainly for debugging purposes
    bool debug; // If true, debug messages will be printed to the console
    // Every single baud rate defined in termios.h or termios-baud.h:
    const std::array<unsigned long, 31> acceptableBaudRates = {B0, B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000};
    long timeout = 1000; // The timeout for read operations. This is in milliseconds. Default is 1000ms (1 second)
public:
    arduinoSerial(std::string port, bool debug=false);
    arduinoSerial();
    ~arduinoSerial();
    void openPort(std::string port);
    void closePort();
    unsigned int available();
    //unsigned int availableForWrite();
    void begin(unsigned long baudRate);
    void end();
    bool find(char target);
    bool find(std::string targetStr);
    bool findUntil(char target, char terminator);
    bool findUntil(std::string targetStr, char terminator);
    void flush();
    float parseFloat();
    long parseInt();
    //int peek();
    void print(std::string str);
    void print(char c);
    void print(int num);
    void print(float num);
    void println(std::string str);
    void println(char c);
    void println(int num);
    void println(float num);
    int read_s(); // read_s() is used instead of read() because read() is already taken by the C library. >:(
    size_t readBytes(char *buffer, size_t length);
    size_t readBytesUntil(char terminator, char *buffer, size_t length);
    std::string readString();
    std::string readStringUntil(char terminator);
    void setTimeout(unsigned long timeout);
    size_t write_s(char byte); // Named like read_s for the same reason
    size_t write_s(char *buffer, size_t size);
};



#define BUFFERSIZE 1024 // Used by functions that are not provided a buffer by the user (e.g. readString()). This could be as small as 1, but i dont think a single kb is too much to ask for.

// Constructor
arduinoSerial::arduinoSerial(std::string port, bool debug){
    this->debug = debug;
    this->ttyName = port;
    if(this->debug){ std::cout << "aurdioSeral object created. ttyName = " << this->ttyName << "\n"; }
}

// Default constructor
arduinoSerial::arduinoSerial(){
    this->debug = false;
    this->ttyName = "/dev/ttyACM0";
    if(this->debug){ std::cout << "aurdioSeral object created. ttyName = " << this->ttyName << "\n"; }
}

// Destructor
arduinoSerial::~arduinoSerial(){
    if(this->debug){ std::cout << "~arduinoSerial() called\n"; }
    this->end();
}

/*
    Change the serial port that the object is using.
    After calling this function, begin() must be called again.
*/
void arduinoSerial::openPort(std::string port){
    this->ttyName = port;
    if(this->debug){ std::cout << "openPort(): ttyName = " << this->ttyName << "\n"; }
}

/*
    * Close the serial port.
*/
void arduinoSerial::closePort(){
    if(this->debug){ std::cout << "close(): Closing serial port\n"; }
    close(this->fd);
}

/*
    * Get the number of bytes (characters) available for reading from the serial port.
*/
unsigned int arduinoSerial::available(){
    int bytesAvailable;
    ioctl(this->fd, FIONREAD, &bytesAvailable);
    if(this->debug){ std::cout << "available(): Detected " << bytesAvailable << " bytes available\n"; }
    return bytesAvailable;
}

/*
    * Get the number of bytes (characters) available for writing in the serial buffer without blocking the write operation.
    * This isnt really something I need to implement, I dont think. I'll leave it here for now, but it may be removed in the future.
unsigned int arduinoSerial::availableForWrite(){
    std::cout << "AvailableForWrite(): Function not implemented, returning -1\n";
    return -1;
}
*/

void arduinoSerial::begin(unsigned long baudRate){
    // Check if the baud rate is valid:
    if(std::find(this->acceptableBaudRates.begin(), this->acceptableBaudRates.end(), baudRate) == this->acceptableBaudRates.end()){
        throw std::invalid_argument("begin(): Invalid baud rate - " + std::to_string(baudRate) + " is not a valid baud rate, see termios.h or termios-baud.h for a list of valid baud rate macros (Hint: format is B<baud rate>)");
        return;
    }
    this->fd = open(this->ttyName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY); // Open the file descriptor
    if(this->fd == -1){
        throw std::runtime_error("begin(): Unable to start the serial port " + this->ttyName);
    }
    if(this->debug){ std::cout << "begin(): Serial port " << this->ttyName << " opened\n"; }
    fcntl(this->fd, F_SETFL, O_NONBLOCK); // Set the file descriptor to nonblocking mode
    struct termios options;
    tcgetattr(this->fd, &options); // Get the current options for the port
    cfsetispeed(&options, baudRate); // Set the baud rates
    cfsetospeed(&options, baudRate);
    /*
        !!!! You may need to change these options to get your serial device working as expected
        !!!! Dont forget about this if you are having weird issues with data not being sent or received properly
    */
    // These options were required to get my ESP32 to work properly (copied from how the esp-idf monitor configured them):
    options.c_iflag &= ~ICRNL; // Disable carriage return to newline mapping
    options.c_iflag &= ~IXON; // Disable XON/XOFF flow control on output
    options.c_oflag &= ~OPOST; // Disable output processing
    options.c_oflag &= ~ONLCR; // Disable mapping NL to CR-NL on output
    options.c_lflag &= ~IEXTEN; // Disable input processing
    options.c_lflag &= ~ECHOK; // Disable kill char erasing the current line
    options.c_lflag &= ~ECHOCTL; // Disable echoing special chars as ^x
    options.c_lflag &= ~ECHOKE; // Disable kill echoed by erasing each char on the line
    // These options were copied from some github repo when i was first making this class:
    options.c_cflag |= (CLOCAL | CREAD); // Enable the receiver and set local mode
    options.c_cflag &= ~CSIZE; // Mask the character size bits
    options.c_cflag |= CS8; // Select 8 data bits
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 Stop bit
    options.c_cflag &= ~CSIZE; // Char size mask
    options.c_cflag |= CS8; // 8 bits
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input mode
    tcsetattr(this->fd, TCSANOW, &options); // Set the new options for the port
    //sleep(1); // Wait for the Arduino to reset
    tcflush(this->fd, TCIFLUSH);
    if(this->debug){ std::cout << "begin(): Serial port " << this->ttyName << " configured. File descriptor: " << this->fd << "\n"; }
}

void arduinoSerial::end(){
    close(this->fd); // Close the file descriptor
    if(this->debug){ std::cout << "end(): Serial port " << this->ttyName << " closed\n"; }
}

/*
    * Reads data from the serial buffer intil the target string is found.
    * Returns true if the target string was found, false otherwise.
    * Times out if the target string is not found within the specified timeout period (this->timeout).
*/
bool arduinoSerial::find(char target){
    auto start = std::chrono::high_resolution_clock::now();
    char c;
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        c = this->read_s();
        if(c == target){
            if(this->debug){ std::cout << "find(): Found target '" << target << "in " << this->ttyName << "\n"; }
            return true;
        }
    }
    if(this->debug){ std::cout << "find(): Timed out while searching for target '" << target << "' in " << this->ttyName << "\n"; }
    return false;
}

/*
    * Reads data from the serial buffer intil the target string is found.
    * Returns true if the target string was found, false otherwise.
    * Times out if the target string is not found within the specified timeout period (this->timeout).
*/
bool arduinoSerial::find(std::string targetStr){
    auto start = std::chrono::high_resolution_clock::now();
    std::string buffer = "";
    char c;
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        c = this->read_s();
        if(c != -1){
            buffer += c;
            if(buffer.find(targetStr) != std::string::npos){
                if(this->debug){ std::cout << "find(): Found target '" << targetStr << "' in " << this->ttyName << "\n"; }
                return true;
            }
            std::cout << "find(): buffer = " << buffer << "\n";
        }
    }
    if(this->debug){ std::cout << "find(): Timed out while searching for target '" << targetStr << "' in " << this->ttyName << "\n"; }
    return false;
}

/*
    * Reads data from the serial buffer intil the target string is found.
    * Returns true if the target string was found, false otherwise.
    * Returns false if the termination character is found
    * Times out if the target string is not found within the specified timeout period (this->timeout).
*/
bool arduinoSerial::findUntil(char target, char terminator){
    auto start = std::chrono::high_resolution_clock::now();
    char c;
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        c = this->read_s();
        if(c == target){
            if(this->debug){ std::cout << "findUntil(): Found target '" << target << "' in " << this->ttyName << "\n"; }
            return true;
        }
        if(c == terminator){
            if(this->debug){ std::cout << "findUntil(): Found terminator '" << terminator << "' in " << this->ttyName << "\n"; }
            return false;
        }
    }
    if(this->debug){ std::cout << "findUntil(): Timed out while searching for target '" << target << "' in " << this->ttyName << "\n"; }
    return false;
}
bool arduinoSerial::findUntil(std::string targetStr, char terminator){
    auto start = std::chrono::high_resolution_clock::now();
    std::string buffer = "";
    char c;
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        c = this->read_s();
        if(c != -1){
            buffer += c;
            if(buffer.find(targetStr) != std::string::npos){
                if(this->debug){ std::cout << "findUntil(): Found target '" << targetStr << "' in " << this->ttyName << "\n"; }
                return true;
            }
            if(c == terminator){
                if(this->debug){ std::cout << "findUntil(): Found terminator '" << terminator << "' in " << this->ttyName << "\n"; }
                return false;
            }
        }
    }
    if(this->debug){ std::cout << "findUntil(): Timed out while searching for target '" << targetStr << "' in " << this->ttyName << "\n"; }
    return false;
}

/*
    * Removes incoming serial data from the serial buffer.
*/
void arduinoSerial::flush(){
    tcflush(this->fd, TCIOFLUSH);
    if(this->debug){ std::cout << "flush(): Flushed " << this->ttyName << "\n"; }
}

/*
    * Looks for the next valid float in the incoming serial.
    * (based on ASCII digits, decimal point, and negative sign).
    * Times out if no valid float is found within the specified timeout period (this->timeout).
*/
float arduinoSerial::parseFloat(){
    auto start = std::chrono::high_resolution_clock::now();
    std::string numStr; // Used to store the number as a string (so we can convert it to a float using std::stof())
    bool allowDecimal = true; // Used to make sure there is only one decimal point in the number
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        char c = this->read_s();
        // First perform all checks which would cause the function to return the current value of numStr. This includes the end of the buffer, and the end of the number.
        if(c == -1){ // Return numStr if end of buffer is reached
            if(this->debug){ std::cout << "parseFloat(): Reached end of buffer, returning " << numStr << "\n"; }
            return (numStr.length() > 0)? std::stof(numStr) : 0; // If numStr is empty, return 0
        }
        if(numStr.length() > 0 && !isdigit(c) && c != '.' && c != '-'){ // Return numStr if numStr.length() > 0 and the current character is not a digit, decimal point, or negative sign
            if(this->debug){ std::cout << "parseFloat(): Found non-digit, non-decimal point, non-negative sign, returning " << numStr << "\n"; }
            return (numStr.length() > 0)? std::stof(numStr) : 0; // If numStr is empty, return 0
        }
        if(c == '.' && !allowDecimal){ // Return numStr if the current character is a decimal point and allowDecimal is false
            if(this->debug){ std::cout << "parseFloat(): Found second decimal point, returning " << numStr << "\n"; }
            return (numStr.length() > 0)? std::stof(numStr) : 0; // If numStr is empty, return 0
        }
        if(c == '-' && numStr.length() > 0){ // Return numStr if the current character is a negative sign and numStr.length() > 0
            if(this->debug){ std::cout << "parseFloat(): Found negative sign in the middle of a number, returning " << numStr << "\n"; }
            return (numStr.length() > 0)? std::stof(numStr) : 0; // If numStr is empty, return 0
        }
        // Now that we know that we dont have to return numStr, we can add the current character to numStr (as long as it is a digit, decimal point, or negative sign)
        if(c == '.'){ allowDecimal = false; }
        if(isdigit(c) || c == '.' || c == '-'){ numStr += c; }

    }
    if(this->debug){ std::cout << "parseFloat(): Timed out while searching for a float, returning " << numStr << "\n"; }
    return (numStr.length() > 0)? std::stof(numStr) : 0; // If numStr is empty, return 0
}

/*
    * Looks for the next valid integer in the incoming serial. Will read until it finds a valid integer.
    * (based on ASCII digits, and negative sign).
*/
long arduinoSerial::parseInt(){
    auto start = std::chrono::high_resolution_clock::now();
    long num = 0;
    char lastC; // Used to keep track of sign of number
    bool sign = true; // True if positive, false if negative
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        char c = this->read_s();
        lastC = c;
        if(isdigit(c)){
            if(lastC == '-'){
                sign = false;
            }
            num = num * 10 + (c - '0');
        }else if(num != 0){
            if(this->debug){ std::cout << "parseInt(): Found integer " << num * (sign? 1 : -1) << "\n"; }
            return num * (sign? 1 : -1);
        }
    }
    if(this->debug){ std::cout << "parseInt(): Timed out while searching for an integer, returning " << num * (sign? 1 : -1) << "\n"; }
    return num * (sign? 1 : -1);
}

/*
    * Returns the next byte of incoming serial data without removing it from the internal serial buffer.
    * WONT IMPLEMENT, pushing a character back into the buffer doesnt seem to work when i tried and i cant be bothered to find the actual solution.
int arduinoSerial::peek(){
    return -1;
}
*/

/*
    * Prints data to the serial port (as human-readable ASCII text ?maybe?)
    * Will need to write a few overloaded functions for different data types
    * Could probably use templates
*/
void arduinoSerial::print(std::string str){
    write(this->fd, str.c_str(), str.length());
    if(this->debug){ std::cout << "print(): Wrote " << str.length() << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::print(char c){
    write(this->fd, &c, 1);
    if(this->debug){ std::cout << "print(): Wrote 1 byte to " << this->ttyName << "\n"; }
}
void arduinoSerial::print(int i){
    std::string str = std::to_string(i);
    write(this->fd, str.c_str(), str.length());
    if(this->debug){ std::cout << "print(): Wrote " << str.length() << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::print(float f){
    std::string str = std::to_string(f);
    write(this->fd, str.c_str(), str.length());
    if(this->debug){ std::cout << "print(): Wrote " << str.length() << " bytes to " << this->ttyName << "\n"; }
}

/*
    * Prints data to the serial port followed by a newline as human-readable ASCII text.
    * Will need to write a few overloaded functions for different data types
*/
void arduinoSerial::println(std::string str){
    int bytes_written =  write(this->fd, str.c_str(), str.length());
    bytes_written += write(this->fd, "\n\r", 2);
    if(this->debug){ std::cout << "println(): Wrote " << bytes_written << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::println(char c){
    int bytes_written = write(this->fd, &c, 1);
    bytes_written += write(this->fd, "\n\r", 2);
    if(this->debug){ std::cout << "println(): Wrote " << bytes_written << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::println(int i){
    std::string str = std::to_string(i);
    int bytes_written = write(this->fd, str.c_str(), str.length());
    bytes_written += write(this->fd, "\n\r", 2);
    if(this->debug){ std::cout << "println(): Wrote " << bytes_written << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::println(float f){
    std::string str = std::to_string(f);
    int bytes_written = write(this->fd, str.c_str(), str.length());
    bytes_written += write(this->fd, "\n\r", 2);
    if(this->debug){ std::cout << "println(): Wrote " << bytes_written << " bytes to " << this->ttyName << "\n"; }
}

/*
    * This function reads a single byte from the serial port.
    * It returns -1 if no data is available.
    * Function is called read_s() because read() is already taken by the C library.
*/
int arduinoSerial::read_s(){
    unsigned char byte;
    int bytesRead = read(this->fd, &byte, 1);
    if(bytesRead == -1){
        if(this->debug){ std::cout << "read_s(): Did not read from serial port " << this->ttyName << " (Returned -1) - Buffer is likely empty\n"; }
        return -1;
    }
    if(bytesRead == 0){
        if(this->debug){ std::cout << "read_s(): ERROR reading from serial port " << this->ttyName << " (Returned 0, EOF) - !!! " << this->ttyName << " is likely not accessible !!!\n"; }
        return -1;
    }
    return byte;
}

/*
    * Reads characters from the serial port into a buffer.
    * The function terminates if the terminator character is read, or if it times out.
    * Returns the number of bytes placed in the buffer (0 means no valid data found).
*/
size_t arduinoSerial::readBytes(char *buffer, size_t length){
    auto start = std::chrono::high_resolution_clock::now();
    size_t bytesRead = 0;
    while(bytesRead != length && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        int byte = this->read_s();
        if(byte != -1){
            buffer[bytesRead] = byte;
            bytesRead++;
        }
    }
    if(this->debug && bytesRead == length){ std::cout << "readBytes(): Finished reading from serial port " << this->ttyName << " (specified number of bytes was read)\n"; }
    if(this->debug && bytesRead != length){ std::cout << "readBytes(): Timed out reading from serial port " << this->ttyName << " (specified number of bytes was not read)\n"; }
    if(this->debug){ std::cout << "readBytes(): Read " << bytesRead << " bytes from serial port " << this->ttyName << "\n"; }
    return bytesRead;
}

/*
    * Reads characters from the serial port into a buffer.
    * The function terminates if the terminator character is read, or if it times out.
    * Returns the number of bytes placed in the buffer (0 means no valid data found).
*/
size_t arduinoSerial::readBytesUntil(char terminator, char *buffer, size_t length){
    auto start = std::chrono::high_resolution_clock::now();
    size_t bytesRead = 0;
    while(bytesRead != length && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        char byte = this->read_s(); // Read the next byte in the serial port using the read_s() function from above
        if(byte != -1){
            buffer[bytesRead] = byte;
            bytesRead++;
        }
        if(byte == terminator){
            if(this->debug){ std::cout << "readBytesUntil(): Terminator character found, stopping read\n"; }
            break;
        }
    }
    if(this->debug && bytesRead == length){ std::cout << "readBytesUntil(): Finished reading from serial port " << this->ttyName << " (specified number of bytes was read)\n"; }
    if(this->debug && bytesRead != length){ std::cout << "readBytesUntil(): Timed out reading from serial port " << this->ttyName << " (specified number of bytes was not read)\n"; }
    if(this->debug){ std::cout << "readBytesUntil(): Read " << bytesRead << " bytes from serial port " << this->ttyName << "\n"; }
    return bytesRead;
}

/*
    * Reads characters from the serial port into a std::string.
    * The function terminates if it times out.
*/
std::string arduinoSerial::readString(){
    auto start = std::chrono::high_resolution_clock::now();
    char buffer[BUFFERSIZE]; // Create a buffer to store the data
    std::string str = ""; // Create a string to return
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        size_t bytesRead = this->readBytes(buffer, BUFFERSIZE); // Read the data into the buffer local to this function
        for(size_t i = 0; i < bytesRead; i++){
            str += buffer[i]; // Add the data to the string
        }
    }
    if(this->debug){ std::cout << "readString(): Timeout reached, read std::string from serial port " << this->ttyName << ", bytes read: " << str.length() << "\n"; }
    return str;
}

/*
    * Reads characters from the serial port into a std::string.
    * The function terminates if the terminator character is read, or if it times out.
*/
std::string arduinoSerial::readStringUntil(char terminator){
    auto start = std::chrono::high_resolution_clock::now();
    std::string str = ""; // Create a string to return
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < this->timeout){
        char byte = this->read_s(); // Read the next byte in the serial port using the read_s() function from above
        if(byte != -1){
            str += byte; // Add the data to the string
        }
        if(byte == terminator){
            if(this->debug){ std::cout << "readStringUntil(): Terminator character found, stopping read\n"; }
            break;
        }
    }
    if(this->debug){ std::cout << "readStringUntil(): Read std::string from serial port " << this->ttyName << ", bytes read: " << str.length() << "\n"; }
    return str;
}

/*
    * Sets the timeout for read operations.
    * The timeout is specified in milliseconds.
    * Default is 1000 ms.
*/
void arduinoSerial::setTimeout(unsigned long timeout){
    this->timeout = timeout;
    if(this->debug){ std::cout << "setTimeout(): Set timeout to " << this->timeout << " ms\n"; }
}

/*
    * Writes binary data to the serial port.
    * Data is sent as a byte or series of bytes.
*/
size_t arduinoSerial::write_s(char byte){
    int bytesWritten = write(this->fd, &byte, 1);
    if(bytesWritten == -1 || bytesWritten == 0){
        if(this->debug){ std::cout << "write_s(): Error writing to serial port " << this->ttyName << " (bytesWritten == -1 or 0)\n"; }
        return -1;
    }
    if(this->debug){ std::cout << "write_s(): Wrote " << bytesWritten << " bytes to " << this->ttyName << "\n"; }
    return bytesWritten;
}

size_t arduinoSerial::write_s(char *buffer, size_t size){
    int bytesWritten = write(this->fd, buffer, size);
    if(bytesWritten == -1 || bytesWritten == 0){
        if(this->debug){ std::cout << "write_s(): Error writing to serial port " << this->ttyName << " (bytesWritten == -1 or 0)\n"; }
        return -1;
    }
    if(this->debug){ std::cout << "write_s(): Wrote " << bytesWritten << " bytes to " << this->ttyName << "\n"; }
    return bytesWritten;
}
