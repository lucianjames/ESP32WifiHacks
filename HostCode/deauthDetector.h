#pragma once

#include <vector>
#include <thread>

#include <imgui.h>

struct deauthInfo{
    unsigned char DST[6];
    unsigned char AP[6];
    int count = 1;
};

class deauthDetector{
private:
    std::string port;
    int baudrate;
    arduinoSerial Serial;
    std::vector<deauthInfo> deauths;
    std::thread detectorThread;

    void detector(){
        this->Serial.write_s('t');
        std::string s;
        while(this->detectorRunning){
            int byte = Serial.read_s();
            if(byte > 0){
                if(byte >= 32 && byte <= 126 || byte == '\n'){
                    std::cout << (char)byte; // Useful for debugging info
                }
                s += byte;
                if(s.ends_with("==BEGIN DEAUTH INFO==")){
                    // Read bytes until "==DEAUTH INFO END==" is found
                    std::string s;
                    std::vector<unsigned char> bytes;
                    while(!s.ends_with("==END DEAUTH INFO==")){
                        int byte = Serial.read_s();
                        if(byte != -1){ // Ignore failed reads
                            bytes.push_back(byte);
                            s += bytes.back();
                        }else{
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
                    }
                    bytes.resize(bytes.size()-19); // Remove "==DEAUTH INFO END==" from the end of the deauth info
                    
                    /*
                        Deauth info format:
                        6 BYTES: DESTINATION
                        6 BYTES: AP MAC
                    */
                    struct deauthInfo deauthFrame;
                    std::cout << "==== TESTING ====" << std::endl;
                    for(int i=0; i<6; i++){
                        deauthFrame.DST[i] = bytes[i];
                        std::cout << std::hex() << deauthFrame.DST[i] << ":";
                        deauthFrame.AP[i+6] = bytes[i+6];
                    }
                    std::cout << "=================" << std::endl;

                    
                    s.clear();
                }
                if(s.size() > 64){
                    s = s.substr(32);
                }
            }else{
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

public:
    bool detectorRunning = false;

    void config(std::string port, int baudrate){
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
        ImGui::Begin("Deauth Detector");
        std::string isRunning = "Detector is ";
        isRunning += this->detectorRunning ? "running" : "not running";
        if(this->detectorRunning){
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        }
        ImGui::Text(isRunning.c_str());
        if(this->detectorRunning){
            ImGui::PopStyleColor();
        }
        ImGui::Dummy(ImVec2(0, 2));
        if(this->detectorRunning){
            if(ImGui::Button("Stop detector")){
                this->stopDetector();
            }
        }else{
            if(ImGui::Button("Start detector")){
                this->startDetector();
            }
        }
        ImGui::Dummy(ImVec2(0, 1));
        ImGui::Text("Listbox goes here");
        ImGui::End();
    }

    void startDetector(){
        this->Serial.openPort(this->port);
        this->Serial.begin(this->baudrate);
        this->detectorRunning = true;
        this->detectorThread = std::thread(&deauthDetector::detector, this);
    }

    void stopDetector(){
        this->detectorRunning = false;
        this->detectorThread.join();
        this->Serial.closePort();
    }
};