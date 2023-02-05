#include "networksList.h"
#include "beaconSniffer.h"

class ESP32Interface{
private:
    std::string port;
    int baudrate;
    networksList networks;
    beaconSniffer sniffer;

public:
    ESP32Interface(std::string port="/dev/ttyUSB0", int baudrate=B115200){
        this->port = port;
        this->baudrate = baudrate;
        sniffer.config(&this->networks, port, baudrate);
    }
    
    ~ESP32Interface(){
    }

    void testFunc(){
        this->sniffer.startSniffer();
    }

    void testFunc2(){
        this->sniffer.stopSniffer();
    }

    void update(){
        this->networks.draw(0, 0, 1, 0.25, ImGuiCond_Once);
        this->sniffer.draw(0, 0.26, 0.25, 0.5, ImGuiCond_Once);
    }

};