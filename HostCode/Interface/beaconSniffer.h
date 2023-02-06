#pragma once

#include <thread>

#include "imtui/imtui.h"

#include "ArduinoSerialIO/arduinoSerial.hpp"
#include "networksList.h"

class beaconSniffer{
private:
    std::string port;
    int baudrate;
    arduinoSerial Serial;
    networksList* networks;
    std::thread snifferThread; // Runs the function that reads the output of the ESP32, parses it, and adds APs to this->networks

    void sniffer(){
        this->Serial.write_s('s'); // Tell the ESP32 to start sniffing
        while(this->snifferRunning){
            // Read until "==BEGIN BEACON==" is found
            std::string s;
            while(s.ends_with("==BEGIN BEACON==") == false){
                int byte = Serial.read_s();
                if(byte > 0){ // Ignore null bytes and failed reads
                    s += byte;
                    if(s.size() > 33){ // Dont waste memory holding onto useless data
                        s = s.substr(16);
                    }
                }else{
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Wait before re-trying. Reduces CPU usage
                }
            }

            // Now read all the bytes until "==END BEACON==" is found
            std::vector<unsigned char> beaconFrame;
            while(s.ends_with("==END BEACON==") == false){
                int byte = Serial.read_s();
                if(byte != -1){ // Ignore failed reads
                    beaconFrame.push_back(byte);
                    s += beaconFrame.back();
                }else{
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
        this->Serial.closePort(); // Close the serial connection so some other function can use it
    }

public:
    bool snifferRunning = false;

    ~beaconSniffer(){
        if(this->snifferRunning){
            this->stopSniffer();
        }
    }

    void config(networksList* networks, std::string port, int baudrate){
        this->networks = networks;
        this->port = port;
        this->baudrate = baudrate;
    }

    // Draws a menu to turn sniffing on/off
    void draw(float wStartXNorm,
              float wStartYNorm,
              float wEndXNorm,
              float wEndYNorm,
              ImGuiCond wCondition=ImGuiCond_Always){
        uiHelper::setNextWindowSizeNormalised(wStartXNorm,
                                              wStartYNorm,
                                              wEndXNorm,
                                              wEndYNorm,
                                              wCondition);
        ImGui::Begin("Beacon sniffer");
        std::string isRunning = "Sniffer is ";
        isRunning += this->snifferRunning ? "running" : "not running";
        ImGui::Text(isRunning.c_str());
        ImGui::Dummy(ImVec2(0, 2));
        if(this->snifferRunning){
            if(ImGui::Button("Stop beacon sniffer")){
                this->stopSniffer();
            }
        }else{
            if(ImGui::Button("Start beacon sniffer")){
                this->startSniffer();
            }   
        }
        ImGui::Dummy(ImVec2(0, 1));
        ImGui::Checkbox("Enable validation", &this->networks->validationEnabled);
        ImGui::End();
    }

    void startSniffer(){
        this->Serial.openPort(this->port);
        this->Serial.begin(this->baudrate);
        this->snifferRunning = true;
        this->snifferThread = std::thread(&beaconSniffer::sniffer, this);
    }

    void stopSniffer(){
        this->snifferRunning = false;
        this->snifferThread.join();
    }
};