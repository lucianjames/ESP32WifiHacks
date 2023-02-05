#pragma

#include <thread>

#include "ArduinoSerialIO/arduinoSerial.hpp"
#include "networksList.h"

class beaconSniffer{
private:
    std::string port;
    int baudrate;
    arduinoSerial Serial;
    networksList* networks;
    std::thread snifferThread; // Runs the function that reads the output of the ESP32, parses it, and adds APs to this->networks
    bool snifferRunning;

    void sniffer(){
        this->Serial.write_s('s'); // Tell the ESP32 to start sniffing
        while(this->snifferRunning){
            // Read until "==BEGIN BEACON==" is found
            std::string s;
            while(s.ends_with("==BEGIN BEACON==") == false){
                int byte = Serial.read_s();
                if(byte > 0){
                    s += byte;
                    if(s.size() > 33){ // Dont waste memory holding onto useless data
                        s = s.substr(16);
                    }
                }
            }

            // Now read all the bytes until "==END BEACON==" is found
            std::vector<unsigned char> beaconFrame;
            while(s.ends_with("==END BEACON==") == false){
                int byte = Serial.read_s();
                if(byte != -1){ // Ignore failed reads
                    beaconFrame.push_back(byte);
                    s += beaconFrame.back();
                }
            }
            beaconFrame.resize(beaconFrame.size()-14); // Remove "==END BEACON==" from the end of the beacon frame

            // Parse BSSID from frame
            // BSSID is 6 bytes long and starts at byte 10
            unsigned char BSSID[6];
            for(int i=0; i<6; i++){
                BSSID[i] = beaconFrame[10+i];
            }

            // Parse SSID from beacon frame
            // Size of SSID is at byte 37, then SSID comes at byte 38 onwards
            std::string SSID;
            int SSIDsize = beaconFrame[37];
            for(int i=0; i<SSIDsize; i++){
                SSID += beaconFrame[38+i];
            }

            // Parse the channel from the beacon frame
            // Stored at byte 38+SSIDsize+12
            int channel = beaconFrame[38+SSIDsize+12];

            // Send the collected beacon info to the networks class
            this->networks->addAP(SSID, BSSID, channel);
        }
        this->Serial.write_s('q'); // Tells the ESP32 to stop the sniffer function
    }

public:
    ~beaconSniffer(){
        if(this->snifferRunning){
            this->stopSniffer();
        }
    }

    void config(networksList* networks, std::string port, int baudrate){
        this->networks = networks;
        this->port = port;
        this->baudrate = baudrate;
        Serial.openPort(port);
        Serial.begin(baudrate);
    }

    void startSniffer(){
        this->snifferRunning = true;
        this->snifferThread = std::thread(&beaconSniffer::sniffer, this);
    }

    void stopSniffer(){
        this->snifferRunning = false;
        this->snifferThread.join();
    }
};