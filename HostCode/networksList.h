#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <algorithm>

#include <imgui.h>

#include "uiHelper.h"

struct accessPoint{
    std::string SSID;
    std::string BSSID_hex;
    unsigned char BSSID[6];
    int channel;
    int beaconsReceived = 1;
    std::string infoStr;
};

struct trafficInfo{
    std::string SRC_BSSID_hex;
    std::string DST_BSSID_hex;
    unsigned char SRC_BSSID[6];
    unsigned char DST_BSSID[6];
    int count = 1;
    std::string infoStr;
};

struct deauthInfo{
    std::string DST_hex;
    std::string AP_hex;
    unsigned char DST[6];
    unsigned char AP[6];
    int count = 1;
    std::string infoStr;
};

class networksList{
private:
    std::vector<accessPoint> networks;
    std::vector<trafficInfo> traffic;
    std::vector<deauthInfo> deauths;
    std::mutex netInfoMutex; // Because another thread is going to be calling addAP()
    int selectedNetwork = 0;
    int selectedTraffic = 0;

    // Used for creating nice readable MAC addresses in the UI:
    template<typename T>
    std::string charBufToHexStr(T buf, int n){
        std::string hexStr;
        for(int i=0; i<n; i++){
            char hex[2];
            sprintf(hex, "%02x", buf[i]);
            hexStr += hex;
            hexStr += ":";
        }
        hexStr.pop_back();
        return hexStr;
    }

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
        ImGui::Text("Networks");
        ImGui::PushItemWidth(-1);
        ImGui::ListBox("##Networks",
                    &this->selectedNetwork,
                    [](void* data, int idx, const char** out_text){
                        std::vector<accessPoint>* networksPointer = (std::vector<accessPoint>*)data; // Convert the void* data to its actual type
                        *out_text = networksPointer->at(idx).infoStr.c_str();
                        return true;
                    },
                    (void*)&this->networks,
                    this->networks.size(),
                    this->networks.size()
        );
        ImGui::PopItemWidth();
        ImGui::Text("Data traffic (0x88 and 0x08 frames)");
        ImGui::PushItemWidth(-1);
        ImGui::ListBox("##Traffic",
                    &this->selectedTraffic,
                    [](void* data, int idx, const char** out_text){
                        std::vector<trafficInfo>* trafficPointer = (std::vector<trafficInfo>*)data;
                        *out_text = trafficPointer->at(idx).infoStr.c_str();
                        return true;
                    },
                    (void*)&this->traffic,
                    this->traffic.size(),
                    this->traffic.size()
        );
        ImGui::PopItemWidth();
        ImGui::Text("Deauths (0xc0 frames)");
        ImGui::PushItemWidth(-1);
        ImGui::ListBox("##Deauths",
                    &this->selectedTraffic,
                    [](void* data, int idx, const char** out_text){
                        std::vector<deauthInfo>* deauthsPointer = (std::vector<deauthInfo>*)data; // Convert the void* data to its actual type
                        *out_text = deauthsPointer->at(idx).infoStr.c_str();
                        return true;
                    },
                    (void*)&this->deauths,
                    this->deauths.size(),
                    this->deauths.size()
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
                n.infoStr = n.SSID + " / " + n.BSSID_hex + " on channel " + std::to_string(n.channel) + " (Received " + std::to_string(n.beaconsReceived) + " beacons)";
                this->netInfoMutex.unlock();
                return;
            }
        }

        // Not found, so add it as a new item in the vector
        accessPoint newAP;
        newAP.SSID = SSID;
        memcpy(newAP.BSSID, BSSID, 6);
        newAP.BSSID_hex = charBufToHexStr(newAP.BSSID, 6);
        newAP.channel = channel;
        newAP.infoStr = newAP.SSID + " / " + newAP.BSSID_hex + " on channel " + std::to_string(newAP.channel) + " (Received 1 beacons)";
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
                t.infoStr = t.SRC_BSSID_hex + " -> " + t.DST_BSSID_hex + " (" + std::to_string(t.count) + " packets detected)";
                this->netInfoMutex.unlock();
                return;
            }
        }

        // Not found, so create new struct and add to this->networks
        trafficInfo newTraffic;

        memcpy(newTraffic.SRC_BSSID, SRC_BSSID, 6);
        memcpy(newTraffic.DST_BSSID, DST_BSSID, 6);
        newTraffic.SRC_BSSID_hex = charBufToHexStr(newTraffic.SRC_BSSID, 6);
        newTraffic.DST_BSSID_hex = charBufToHexStr(newTraffic.DST_BSSID, 6);
        newTraffic.infoStr = newTraffic.SRC_BSSID_hex + " -> " + newTraffic.DST_BSSID_hex + " (1 packets detected)";
        this->traffic.push_back(newTraffic);

        this->netInfoMutex.unlock();
    }

    void addDeauth(unsigned char* DST,
                   unsigned char* AP){
        this->netInfoMutex.lock();

        // Check if an entry already exists
        for(auto& d : this->deauths){
            bool dstMatch = true;
            bool apMatch = true;
            for(int i=0; i<6; i++){
                if(d.DST[i] != DST[i]){
                    dstMatch = false;
                    break;
                }
                if(d.AP[i] != AP[i]){
                    apMatch = false;
                    break;
                }
            }
            if(dstMatch && apMatch){
                d.count++;
                d.infoStr = d.DST_hex + " deauthed by " + d.AP_hex + " (" + std::to_string(d.count) + " deauths detected)";
                this->netInfoMutex.unlock();
                return;
            }
        }

        // Not found, so add to this->deauths
        deauthInfo newDeauth;
        memcpy(newDeauth.DST, DST, 6);
        memcpy(newDeauth.AP, AP, 6);
        newDeauth.DST_hex = charBufToHexStr(newDeauth.DST, 6);
        newDeauth.AP_hex = charBufToHexStr(newDeauth.AP, 6);
        newDeauth.infoStr = newDeauth.DST_hex + " deauthed by " + newDeauth.AP_hex + " (1 deauths detected)";
        this->deauths.push_back(newDeauth);

        this->netInfoMutex.unlock();
    }

    accessPoint getSelectedAccessPoint(){
        if(this->networks.size() == 0 || this->selectedNetwork >= this->networks.size() || this->selectedNetwork < 0){
            accessPoint emptyAP;
            emptyAP.SSID = "NONE";
            return emptyAP;
        }
        return this->networks[this->selectedNetwork];
    }
};