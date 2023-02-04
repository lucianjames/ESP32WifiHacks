#include <string>
#include <vector>

#include "ArduinoSerialIO/arduinoSerial.hpp"

arduinoSerial Serial("/dev/ttyUSB0");

void SSID_BSSID_SCAN(){
    int beacons = 0;
    while(1){
        std::cout << "Waiting for next beacon frame....\n";
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

        // Now the bytes up to "==END BEACON==" are the beacon frame
        std::vector<unsigned char> beaconFrame;
        while(s.ends_with("==END BEACON==") == false){
            int byte = Serial.read_s();
            if(byte != -1){ // Ignore failed reads
                beaconFrame.push_back(byte);
                s += beaconFrame.back();
            }
        }
        beaconFrame.resize(beaconFrame.size()-14); // Remove "==END BEACON==" from the end of the beacon frame


        // Print beacon frame info
        std::cout << "==BEGIN BEACON INFO==\n";

        // Print hex of beacon frame
        std::cout << "BEACON FRAME HEX:\n";
        for(unsigned char c : beaconFrame){
            printf("%02X ", c);
        }
        std::cout << std::endl;

        std::cout << "BEACON FRAME ASCII:\n";
        for(unsigned char c : beaconFrame){
            if(c >= 32 && c <= 126){
                printf("%c", c);
            }else{
                printf(".");
            }
        }
        std::cout << std::endl;

        // Parse SSID from beacon frame
        // Size of SSID is stored at byte 37
        // SSID is from byte 38 to byte 38+SSID_size
        int SSID_size = beaconFrame[37];
        std::string SSID;
        for(int i=0; i<SSID_size; i++){
            SSID += beaconFrame[38+i];
        }
        printf("BEACON SSID (%d bytes):\n%s\n", SSID_size, SSID.c_str());

        // Parse BSSID from beacon frame
        // The BSSID is 6 bytes long and is stored at byte 10
        std::array<unsigned char, 6> BSSID;
        for(int i=0; i<6; i++){
            BSSID[i] = beaconFrame[10+i];
        }
        printf("BEACON BSSID:\n");
        for(int i=0; i<6; i++){
            printf("%02X ", BSSID[i]);
        }
        std::cout << std::endl;

        // Parse the channel from the beacon frame
        // The channel is stored at byte 12+38+SSID_size+13
        int channel = beaconFrame[12+38+SSID_size+12];
        printf("BEACON ON CHANNEL:\n%d\n", channel);

        std::cout << "==END BEACON INFO==" << std::endl;

        if(beacons++ > 10){
            Serial.write_s('q');
            return;
        }
    }
}

int main(){
    Serial.begin(B115200);
    Serial.setTimeout(10000);
    Serial.write_s('s');

    SSID_BSSID_SCAN();

    return 0;
}