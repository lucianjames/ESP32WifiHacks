#pragma once

#include <thread>

#include "imtui/imtui.h"

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
        ImGui::Text("NOT IMPLEMENTED DUMMY UI");
        ImGui::Dummy(ImVec2(0, 2));
        std::string isRunning = "Deauther is ";
        isRunning += this->deautherRunning ? "running" : "not running";
        ImGui::Text(isRunning.c_str());
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