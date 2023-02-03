/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"

// Just print readable stuff to the console for now
// Will create stuff that can be parsed by the interface program later
void wifi_promiscuous(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf; // Cast the void type into an actual usable packet type
    printf("Received packet!\n");
    printf("HEX:\n");
    // Print the packet as hex
    for(int i=0; i<pkt->rx_ctrl.sig_len; i++) {
        printf("%02x ", pkt->payload[i]);
    }
    printf("\n\nASCII:\n");
    // Print anything that is printable
    for (int i = 0; i < pkt->rx_ctrl.sig_len; i++) {
        if(pkt->payload[i] >= 32 && pkt->payload[i] <= 126){
            printf("%c", pkt->payload[i]);
        }else{
            printf(".");
        }
    }
    printf("\n\n\n");
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
        printf("\n\nSwitched to channel %d\n\n", ch);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void){

    // System & WiFi
    nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //ESP_ERROR_CHECK(esp_wifi_set_country(WIFI_COUNTRY_EU));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    
    esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
    esp_wifi_set_promiscuous(true);

    printf("Hello world!\n");

    // Create a task to handle the channel hopping
    xTaskCreate(&channelHopper, "channelHopper", 2048, NULL, 5, NULL);


    while(1){
        vTaskDelay(portMAX_DELAY); // Sleep for 7 weeks :D
    }
}
