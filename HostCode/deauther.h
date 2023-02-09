#pragma once

#include <thread>

#include <imgui.h>

#include "ArduinoSerialIO/arduinoSerial.hpp"
#include "networksList.h"

class deauther{
private:
    std::string port;
    int baudrate;
    arduinoSerial Serial;
    networksList* networks;
    std::thread deautherThread;
    accessPoint target;

    void deauth(){
        // Send the deauth command to the ESP
        std::vector<char> deauthCmd;
        deauthCmd.push_back('d');
        for(auto c : this->target.BSSID){
            deauthCmd.push_back(c);
        }
        deauthCmd.push_back(this->target.channel);
        Serial.write_s(deauthCmd.data(), deauthCmd.size());
        // Wait until this->deautherRunning is set to false
        while(this->deautherRunning){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        this->Serial.write_s('q'); // Sending 'q' to the ESP will tell it to stop deauthing
    }

public:
    bool deautherRunning = false;

    ~deauther(){
        if(this->deautherRunning){
            this->stopDeauther();
        }
    }

    void config(networksList* networks, std::string port, int baudrate){
        this->networks = networks;
        this->port = port;
        this->baudrate = baudrate;
    }

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
        ImGui::Begin("Deauther");
        std::string isRunning = "Deauther is ";
        isRunning += this->deautherRunning ? "running" : "not running";
        if(this->deautherRunning){
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        }
        ImGui::Text(isRunning.c_str());
        if(this->deautherRunning){
            ImGui::PopStyleColor();
        }
        ImGui::Dummy(ImVec2(0, 2));
        if(this->deautherRunning){
            if(ImGui::Button("Stop deauthing")){
                this->stopDeauther();
            }
        }else{
            if(ImGui::Button("Start deauthing selected network")){
                this->startDeauther();
            }
        }
        ImGui::End();
    }

    void startDeauther(){
        this->target = this->networks->getSelectedAccessPoint();
        this->Serial.openPort(this->port);
        this->Serial.begin(this->baudrate);
        this->deautherRunning = true;
        this->deautherThread = std::thread(&deauther::deauth, this);
    }

    void stopDeauther(){
        this->deautherRunning = false;
        this->deautherThread.join();
    }

};