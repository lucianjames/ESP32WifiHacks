#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "nvs_flash.h"

#include "beaconSniffer.h"
#include "beaconSpammer.h"
#include "deauther.h"
#include "espFrameSanityCheckHack.h"

void app_main(void){
    //Initialize NVS 
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Wait for command from host
    // Commands:
    // 's' - Start beacon sniffing. Exits on 'q'
    // "b<SSID1>0x03<SSID2>0x03<SSIDn>\n" - Start sending out beacons with the given SSIDs. Exits on 'q'
    while(1){
        int c = getchar();
        if(c == 's'){
            runBeaconSniffer();
        }
        else if(c == 'b'){
            runBeaconSpammer();
        }else if(c == 'd'){
            runDeauther();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms delay to reduce CPU usage
    }
}
