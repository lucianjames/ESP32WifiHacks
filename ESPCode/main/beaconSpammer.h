#include "wifiInit.h"
#include "frameContents.h"
#include "esp_log.h"

struct apInfo{
    char* ssid;
    int ssidLen;
    uint8_t channel;
    uint8_t bssid[6];
};

#define BEACON_SEND_N 4 // Experimental, send 4 frames in a row for each "AP", seems to cause the AP to get picked up better

/*
    This declaration gives access to the esp_wifi_80211_tx function,
    which gives the ability to send raw arbitrary 802.11 frames.
*/
esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

// These variables are global so they can be freed from wherever
struct apInfo* APs;
char** ssids;

void beaconSpammer(){
    // Read the full command from the host
    // This will give a list of SSIDs to spam
    int cmdLen = 256;
    char* cmd = (char*)malloc(cmdLen * sizeof(char));
    int pos = 0;
    int c = 0;
    while(c != '\n'){
        do{
            c=getchar();
        }while(c==-1);
        cmd[pos] = c;
        pos++;
        if(pos >= cmdLen-1){
            cmdLen += 256;
            cmd = (char*)realloc(cmd, cmdLen * sizeof(char));
        }
    }
    cmd[pos-1] = '\0'; // Remove the newline character

    // Parse the command
    int ssidCount = 0;
    int ssidArrLen = 10;
    ssids = malloc(ssidArrLen * sizeof(char*));
    char delim[] = {0x03, 0x00};
    char* ssid = strtok(cmd, delim);
    while(ssid != NULL){
        ssids[ssidCount] = (char*)malloc((strlen(ssid)+1) * sizeof(char));
        strcpy(ssids[ssidCount], ssid);
        ssidCount++;
        if(ssidCount >= ssidArrLen){
            ssidArrLen += 10;
            ssids = (char**)realloc(ssids, ssidArrLen * sizeof(char*));
        }
        ssid = strtok(NULL, delim);
    }
    free(cmd);

    // Create an array of apInfo structs, one for each SSID
    APs = malloc(sizeof(struct apInfo) * ssidCount);
    for(int i=0; i<ssidCount; i++){
        ESP_LOGI("beaconSpammer", "SSID %d: %s", i, ssids[i]);
        APs[i].ssid = ssids[i];
        APs[i].ssidLen = strlen(ssids[i]);
        // Generate a random BSSID
        for(int j=0; j<6; j++){
            APs[i].bssid[j] = rand() % 256;
        }
        APs[i].channel = 11; // The ap_config thing is set to 11, so ill just put them all on 11 for now
    }

    int apIdx = 0;
    while(1){
        // Assemble a beacon frame for the current AP
        uint8_t* frame = (uint8_t*)malloc(256*sizeof(uint8_t));
        uint8_t* p = frame;
        memcpy(p, beaconFrame_p1, sizeof(beaconFrame_p1));
        p += sizeof(beaconFrame_p1);
        memcpy(p, APs[apIdx].bssid, 6);
        p += 6;
        memcpy(p, APs[apIdx].bssid, 6);
        p += 6;
        memcpy(p, beaconFrame_p2, sizeof(beaconFrame_p2));
        p += sizeof(beaconFrame_p2);
        *p = APs[apIdx].ssidLen;
        p++;
        //memcpy(p, APs[apIdx].ssid, APs[apIdx].ssidLen);
        // The above line doesnt work, so heres the below line instead:
        for(int i=0; i<APs[apIdx].ssidLen; i++){
            *p = APs[apIdx].ssid[i];
            p++;
        }
        memcpy(p, beaconFrame_p3, sizeof(beaconFrame_p3));
        p += sizeof(beaconFrame_p3);
        *p = APs[apIdx].channel;
        p++;
        memcpy(p, beaconFrame_p4, sizeof(beaconFrame_p4));
        p += sizeof(beaconFrame_p4);

        for(int i=0; i<BEACON_SEND_N; i++){
            esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, p-frame, false);
            if(err != ESP_OK){
                printf("Error sending beacon frame: %d\r\n", err);
            }
        }
        free(frame); // Free the frame memory
        vTaskDelay(10 / portTICK_PERIOD_MS); // Changing this from 10 to 9 causes it to not work. Fun :)
        apIdx = (apIdx+1)%ssidCount;
    }
}

TaskHandle_t beaconSpammerTaskHandle;

void runBeaconSpammer(){
    wifiInit(true);
    xTaskCreate(beaconSpammer, "beaconSpammer", 4096*4, NULL, 5, &beaconSpammerTaskHandle);
    vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for the task to start
    // Once we can be sure that the command has been read, its safe to read for the 'q' character
    while(getchar() != 'q'){ // Wait for 'q' to be sent by host
        vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms delay reduces cpu usage
    }
    vTaskDelete(beaconSpammerTaskHandle);
    // Free the memory malloc'd by beaconSpammer
    free(ssids);
    free(APs);
    wifiDeInit();
}
