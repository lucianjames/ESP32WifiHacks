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
    // "b<SSID1>0x00 0x00<SSID2>0x00 0x00<SSIDn>0x00\n" - Start sending out beacons with the given SSIDs. Exits on 'q'

    while(1){
        char c = getchar();
        if(c == 's'){
            runBeaconSniffer();
        } else if(c == 'b'){
            printf("Beacon spam cmd recvd\n");
            // Parse SSIDs. Store in malloc'd array of arrays
            // Once '\n' is received, pass the SSIDs to the beacon spammer function
            int ssidCount = 0; // Number of SSIDs read
            int ssidSpace = 10; // Number of SSIDs that can be stored in the array
            char** ssids = malloc(ssidSpace * sizeof(char*));
            while(1){
                printf("Reading SSID %d\n", ssidCount);
                ssids[ssidCount] = malloc(33 * sizeof(char)); // 32 is max SSID size (+1 byte 0x00 terminator), not worried about using extra bytes allocating it all even if unused
                // Read until 0x00 is found (end of SSID)
                // Currently assuming that the host validates everything, invalid SSIDs (such as too long) will cause the ESP32 to crash :)
                int pos = 0;
                c = 0x01;
                while(c != 0x00){
                    c = getchar();
                    printf("Read char: %d\n", c);
                    ssids[ssidCount][pos] = c;
                    pos++;
                }
                printf("Read SSID: %s\n", ssids[ssidCount]);
                ssidCount++;
                if(getchar() == '\n'){ // If the next character is a newline, we're done reading SSIDs, if there are more SSIDs to read, this will be an 0x00
                    // Done reading SSIDs
                    printf("Done reading SSIDs\n");
                    break;
                } // Else, more SSIDs to read
                printf("More SSIDs to read\n");
                // If we've reached the max number of SSIDs, allocate more space
                if(ssidCount >= ssidSpace){
                    ssidSpace += 10;
                    ssids = realloc(ssids, ssidSpace * sizeof(char*));
                }
            }
            // Pass the SSIDs to the beacon spammer function
            // runBeaconSpammer(ssids, ssidCount);
            // For now (debugging), just print the SSIDs
            printf("Done!, read %d SSIDs\n", ssidCount);
            for(int i = 0; i < ssidCount; i++){
                printf("%s\n", ssids[i]);
            }

        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms delay to reduce CPU usage
    }
}
