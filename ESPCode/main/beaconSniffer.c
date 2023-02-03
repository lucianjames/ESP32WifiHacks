#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"

// Callback function for promiscuous mode
// This function sends beacon frames back to the host pc, so that they can be parsed and stored
void wifi_promiscuous(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf; // Cast the void type into an actual usable packet type
    if(pkt->payload[0] == 0x80){
        printf("==BEGIN BEACON==");
        // Print the packet as hex
        for(int i=0; i<pkt->rx_ctrl.sig_len; i++) {
            printf("%c", pkt->payload[i]);
        }
        printf("==END BEACON==");
    }
}

// Sets the channel of the ESP32
// !!! Sets the callback to wifi_promiscuous() !!!
// Could pass a function pointer as a parameter to make it more flexible
void setChannel(int ch) {
    ch = (ch>13 || ch<1) ? 1 : ch; // Make sure the channel is valid (1-13)
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous); // Need to re-set the callback function after disabling promiscuous mode
    esp_wifi_set_promiscuous(true);
}

void channelHopper(void* pvParameters) {
    int ch = 1;
    while(1) {
        ch = (ch+1) % 14;
        setChannel(ch);
        vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 100ms
    }
}

void app_main(void){
    // System & WiFi setup
    nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    
    // Set the callback function and start promiscuous mode
    esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
    esp_wifi_set_promiscuous(true);

    // Create a task to handle the channel hopping
    xTaskCreate(&channelHopper, "channelHopper", 2048, NULL, 5, NULL);
}
