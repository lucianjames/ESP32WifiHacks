#include "ArduinoSerialIO/arduinoSerial.hpp"

class ESP32Interface{
private:
    arduinoSerial Serial;
public:
    ESP32Interface(){ // Default constructor opens sensible defaults :)
        Serial.openPort("/dev/ttyUSB0");
        Serial.begin(B115200);
    }

    ESP32Interface(std::string port, int baudrate=B115200){
        Serial.openPort(port);
        Serial.begin(baudrate);
    }
    
    ~ESP32Interface(){
        Serial.closePort();
    }

};