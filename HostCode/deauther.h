#pragma once

#include <thread>

#include <imgui.h>

#include "ArduinoSerialIO/arduinoSerial.hpp"
#include "networksList.h"

class deauther{
private:
    std::string port;
    int baudrate;
    int selectedTraffic = 0;
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
            // Print anything the ESP sends (for debugging purposes)
            int c = this->Serial.read_s();
            if(c != -1 && (c >= 32 && c <= 126 || c == '\n')){
                std::cout << (char)c;
            }
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
            if(ImGui::Button("Stop deauthing broadcast")){
                this->stopDeauther();
            }
        }else{
            if(ImGui::Button("Start deauthing selected network (to broadcast)")){
                this->startDeauther();
            }
        }

        std::vector<netConnection> traffic = this->networks->getSelectedTraffic();
        std::vector<std::string> trafficStrings;
        for(auto t : traffic){
            trafficStrings.push_back(t.BSSID_hex);
        }
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::Text("MAC addresses detected using the selected network:");
        ImGui::PushItemWidth(-1);
        ImGui::ListBox("##Traffic",
                    &this->selectedTraffic,
                    [](void* data, int idx, const char** out_text){
                        std::vector<std::string>* trafficStrPointer = (std::vector<std::string>*)data; // Convert the void* data to its actual type
                        *out_text = trafficStrPointer->at(idx).c_str();
                        return true;
                    },
                    (void*)&trafficStrings,
                    trafficStrings.size(),
                    trafficStrings.size()
        );
        ImGui::PopItemWidth();

        ImGui::Button("Deauth selected network (targeting displayed MAC addresses)");

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