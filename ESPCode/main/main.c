#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "beaconSniffer.h"

void app_main(void){
    // System & WiFi setup
    nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    
    // Wait for command from host
    // Commands:
    // 's' - Start beacon sniffing. Exits on 'q'
    // "b;SSID1;SSID2;...;SSIDn;\n" - Start sending out beacons with the given SSIDs. Exits on 'q'

    while(1){
        char c = getchar();
        if(c == 's'){
            runBeaconSniffer();
        } else if(c == 'b'){
            // Parse SSIDs. Store in malloc'd array of arrays
            // Once '\n' is received, pass the SSIDs to the beacon spammer function
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms delay to reduce CPU usage
    }
}
