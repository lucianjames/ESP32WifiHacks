#include "networksList.h"
#include "beaconSniffer.h"
#include "beaconSpammer.h"
#include "deauther.h"

class ESP32Interface{
private:
    std::string port;
    int baudrate;
    networksList networks;
    beaconSniffer sniffer;
    beaconSpammer spammer;
    deauther deauth;

public:
    ESP32Interface(std::string port="/dev/ttyUSB0", int baudrate=B115200){
        this->port = port;
        this->baudrate = baudrate;
        sniffer.config(&this->networks, port, baudrate);
        spammer.config(port, baudrate);
        deauth.config(&this->networks, port, baudrate);
    }
    
    ~ESP32Interface(){
    }

    void update(){
        this->networks.draw(0, 0, 1, 0.5, ImGuiCond_Once);
        this->sniffer.draw(0, 0.56, 0.25, 0.75, ImGuiCond_Once);
        this->spammer.draw(0.26, 0.56, 0.5, 0.75, ImGuiCond_Once);
        this->deauth.draw(0.51, 0.56, 0.75, 0.75, ImGuiCond_Once);
    }

};