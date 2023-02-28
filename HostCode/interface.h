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
    std::mutex espMutex;

public:

    ESP32Interface(std::string port="/dev/ttyUSB0", int baudrate=B115200){
        this->port = port;
        this->baudrate = baudrate;
        this->sniffer.config(&this->networks, port, baudrate, &espMutex);
        this->spammer.config(port, baudrate, &espMutex);
        this->deauth.config(&this->networks, port, baudrate, &espMutex);
    }
    
    ~ESP32Interface(){
    }

    void update(){
        this->networks.draw(0, 0, 1, 0.75, ImGuiCond_Always);
        this->sniffer.draw(0, 0.76, 0.33, 1, ImGuiCond_Always);
        this->spammer.draw(0.34, 0.76, 0.66, 1, ImGuiCond_Always);
        this->deauth.draw(0.67, 0.76, 1, 1, ImGuiCond_Always);
    }

};