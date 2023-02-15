#pragma once

#include <vector>
#include <string>
#include <mutex>

#include <imgui.h>

#include "uiHelper.h"

struct accessPoint{
    std::string SSID;
    std::string BSSID_hex;
    unsigned char BSSID[6];
    int channel;
    int beaconsReceived = 1;
};

struct trafficInfo{
    std::string SRC_BSSID_hex;
    std::string DST_BSSID_hex;
    unsigned char SRC_BSSID[6];
    unsigned char DST_BSSID[6];
    int count = 1;
};

class networksList{
private:
    std::vector<accessPoint> networks;
    std::vector<trafficInfo> traffic;
    std::mutex netInfoMutex; // Because another thread is going to be calling addAP()
    int selectedNetwork = 0;
    int selectedTraffic = 0;

public:
    bool validationEnabled = true;

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
        ImGui::Begin("Networks");

        this->netInfoMutex.lock();
        /*
            This isnt a very efficient approach, but theres no actual reason to optimise it
        */
        std::vector<std::string> networkInfoStrings; // Have to create this because the listbox function works based on pointers
        if(this->networks.size() == 0){
            networkInfoStrings.push_back("No networks discovered");
        }else{
            for(auto n : this->networks){
                networkInfoStrings.push_back(n.SSID + " / " + n.BSSID_hex + " on channel " + std::to_string(n.channel) + ". Received " + std::to_string(n.beaconsReceived) + " beacons");
            }
        }
        std::vector<std::string> trafficInfoStrings;
        if(this->traffic.size() == 0){
            trafficInfoStrings.push_back("No traffic discovered");
        }else{
            for(auto t : this->traffic){
                trafficInfoStrings.push_back(t.SRC_BSSID_hex + " -> " + t.DST_BSSID_hex + " (" + std::to_string(t.count) + " packets detected)");
            }
        }
        
        this->netInfoMutex.unlock();
        ImGui::Text("Networks");
        ImGui::PushItemWidth(-1);
        ImGui::ListBox("##Networks",
                    &this->selectedNetwork,
                    [](void* data, int idx, const char** out_text){
                        std::vector<std::string>* networkStrPointer = (std::vector<std::string>*)data; // Convert the void* data to its actual type
                        *out_text = networkStrPointer->at(idx).c_str();
                        return true;
                    },
                    (void*)&networkInfoStrings,
                    networkInfoStrings.size(),
                    networkInfoStrings.size()
        );
        ImGui::PopItemWidth();
        ImGui::Text("Data traffic (0x88 and 0x08 frames)");
        ImGui::PushItemWidth(-1);
        ImGui::ListBox("##Traffic",
                    &this->selectedTraffic,
                    [](void* data, int idx, const char** out_text){
                        std::vector<std::string>* trafficStrPointer = (std::vector<std::string>*)data; // Convert the void* data to its actual type
                        *out_text = trafficStrPointer->at(idx).c_str();
                        return true;
                    },
                    (void*)&trafficInfoStrings,
                    trafficInfoStrings.size(),
                    trafficInfoStrings.size()
        );
        ImGui::PopItemWidth();
        ImGui::End();
    }

    void addAP(std::string SSID,
               unsigned char* BSSID,
               int channel){
        if(this->validationEnabled){
            // Some basic validation:
            if(channel < 1 || channel > 14){ // If the channel is invalid, the packet is probably corrupt, so definitely dont add it
                return;
            }
            for(auto c : SSID){ // Dont add any APs with non-printable characters in the SSID
                if(c < 32 || c > 126){
                    return;
                }
            }
        }

        this->netInfoMutex.lock();

        // Check if the AP already exists in this->networks, if so, just increment its beacon counter
        for(auto& n : this->networks){
            bool BSSIDMatch = true; // Because using pointers, have to go through every element manually to do a comparison
            for(int i=0; i<6; i++){
                if(BSSID[i] != n.BSSID[i]){
                    BSSIDMatch = false;
                    break;
                }
            }
            if(n.SSID == SSID && BSSIDMatch && n.channel == channel){
                n.beaconsReceived++;
                this->netInfoMutex.unlock();
                return;
            }
        }

        // Not found, so add it as a new item in the vector
        accessPoint newAP;
        newAP.SSID = SSID;
        for(int i=0; i<6; i++){
            newAP.BSSID[i] = BSSID[i];
            char hex[2];
            sprintf(hex, "%02x", BSSID[i]);
            newAP.BSSID_hex += hex;
            newAP.BSSID_hex += ":";
        }
        newAP.BSSID_hex.pop_back();
        newAP.channel = channel;
        this->networks.push_back(newAP);

        this->netInfoMutex.unlock();
    }

    void addTraffic(unsigned char* SRC_BSSID,
                    unsigned char* DST_BSSID){
        this->netInfoMutex.lock();

        // Check if an entry already exists
        for(auto& t : this->traffic){
            bool srcMatch = true;
            bool dstMatch = true;
            for(int i=0; i<6; i++){
                if(t.DST_BSSID[i] != DST_BSSID[i]){
                    dstMatch = false;
                    break;
                }
                if(t.SRC_BSSID[i] != SRC_BSSID[i]){
                    srcMatch = false;
                    break;
                }
            }
            if(srcMatch && dstMatch){
                t.count++;
                this->netInfoMutex.unlock();
                return;
            }
        }

        // Not found, so create new struct and add to this->networks
        trafficInfo newTraffic;
        for(int i=0; i<6; i++){
            newTraffic.SRC_BSSID[i] = SRC_BSSID[i];
            newTraffic.DST_BSSID[i] = DST_BSSID[i];
            char hex[2];
            sprintf(hex, "%02x", SRC_BSSID[i]);
            newTraffic.SRC_BSSID_hex += hex;
            newTraffic.SRC_BSSID_hex += ":";
            sprintf(hex, "%02x", DST_BSSID[i]);
            newTraffic.DST_BSSID_hex += hex;
            newTraffic.DST_BSSID_hex += ":";
        }
        newTraffic.DST_BSSID_hex.pop_back();
        newTraffic.SRC_BSSID_hex.pop_back();
        this->traffic.push_back(newTraffic);

        this->netInfoMutex.unlock();
    }

    accessPoint getSelectedAccessPoint(){
        return this->networks[this->selectedNetwork];
    }
};