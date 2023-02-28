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
    std::mutex* espMutex;

    std::vector<unsigned char> readUntilStr(std::string term){
        std::string s;
        std::vector<unsigned char> bytes;
        while(!s.ends_with(term)){
            int byte = this->Serial.read_s();
            if(byte != -1){ // Ignore failed reads
                bytes.push_back(byte);
                s += (unsigned char)byte;
            }else{
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        bytes.resize(bytes.size()-term.size());
        return bytes;
    }

    void readParseBeacon(){
        // Now read all the bytes until "==END BEACON==" is found
        std::vector<unsigned char> beaconFrame = this->readUntilStr("==END BEACON==");

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
        std::vector<unsigned char> trafficInfo = this->readUntilStr("==END TRAFFIC INFO==");

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

    void readParseDeauth(){
        // Read bytes until "==DEAUTH INFO END==" is found
        std::vector<unsigned char> deauthInfo = this->readUntilStr("==END DEAUTH INFO==");

        /*
            Deauth info format:
            6 BYTES: DESTINATION
            6 BYTES: AP MAC
        */
        unsigned char* DST = new unsigned char[6];
        unsigned char* AP = new unsigned char[6];
        for(int i=0; i<6; i++){
            DST[i] = deauthInfo[i];
            AP[i] = deauthInfo[i+6];
        }

        this->networks->addDeauth(DST, AP);
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
                }else if(s.ends_with("==BEGIN DEAUTH INFO==")){
                    this->readParseDeauth();
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

    void config(networksList* networks, std::string port, int baudrate, std::mutex* m){
        this->networks = networks;
        this->port = port;
        this->baudrate = baudrate;
        this->espMutex = m;
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
        ImGui::Begin("Network sniffer");
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
            if(ImGui::Button("Stop sniffer")){
                this->stopSniffer();
            }
        }else{
            if(ImGui::Button("Start sniffer")){
                this->startSniffer();
            }   
        }
        ImGui::Dummy(ImVec2(0, 1));
        ImGui::Checkbox("Enable validation", &this->networks->validationEnabled);
        ImGui::End();
    }

    void startSniffer(){
        if(!this->espMutex->try_lock()){
            return;
        }
        this->Serial.openPort(this->port);
        this->Serial.begin(this->baudrate);
        this->snifferRunning = true;
        this->snifferThread = std::thread(&beaconSniffer::sniffer, this);
    }

    void stopSniffer(){
        this->snifferRunning = false;
        this->snifferThread.join();
        this->Serial.closePort();
        this->espMutex->unlock();
    }
};