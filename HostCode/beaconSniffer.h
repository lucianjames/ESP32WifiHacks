#pragma once

#include <thread>

#include <imgui.h>

#include "ArduinoSerialIO/arduinoSerial.hpp"
#include "networksList.h"

class beaconSniffer{
private:
    std::string port;
    int baudrate;
    arduinoSerial Serial;
    networksList* networks;
    std::thread snifferThread; // Runs the function that reads the output of the ESP32, parses it, and adds APs to this->networks


    void readParseBeacon(){
        // Now read all the bytes until "==END BEACON==" is found
        std::string s;
        std::vector<unsigned char> beaconFrame;
        while(!s.ends_with("==END BEACON==")){
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

    void readParseTrafficInfo(){
        // Read all bytes until "==END TRAFFIC INFO==" is found
        std::string s;
        std::vector<unsigned char> trafficInfo;
        while(!s.ends_with("==END TRAFFIC INFO==")){
            int byte = Serial.read_s();
            if(byte != -1){ // Ignore failed reads
                trafficInfo.push_back(byte);
                s += trafficInfo.back();
            }else{
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        trafficInfo.resize(trafficInfo.size()-18); // Remove "==END TRAFFIC INFO==" from the end of the traffic info
        // Format of the traffic info:
        // <SOURCE MAC (6 bytes)><DESTINATION MAC (6 bytes)>

        unsigned char sourceMAC[6];
        unsigned char destinationMAC[6];
        for(int i=0; i<6; i++){
            sourceMAC[i] = trafficInfo[i];
            destinationMAC[i] = trafficInfo[6+i];
        }

        this->networks->addTraffic(sourceMAC, destinationMAC);
    }

    void sniffer(){
        this->Serial.write_s('s'); // Tell the ESP32 to start sniffing
        std::string s;
        while(this->snifferRunning){
            int byte = Serial.read_s();
            if(byte > 0){ // Ignore null bytes and failed reads
                if(byte >= 32 && byte <= 126 || byte == '\n'){
                    std::cout << (char)byte; // Useful for getting debug info
                }
                s += byte; // Appends byte as a char
                if(s.ends_with("==BEGIN BEACON==")){
                    this->readParseBeacon();
                    s.clear();
                }else if(s.ends_with("==BEGIN TRAFFIC INFO==")){
                    this->readParseTrafficInfo();
                    s.clear();
                }
                if(s.size() > 64){ // Dont waste memory holding onto useless data
                    s = s.substr(32);
                }
            }else{
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Wait before re-trying. Reduces CPU usage
            }
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
        if(this->snifferRunning){
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        }
        ImGui::Text(isRunning.c_str());
        if(this->snifferRunning){
            ImGui::PopStyleColor();
        }
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