void beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
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

void beaconSnifferChannelHopper(void* pvParameters) {
    int ch = 1;
    while(1) {
        ch = ch % 13 + 1; // Cycle through channels 1-13
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
        ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback)); // Need to re-set the callback function after disabling promiscuous mode
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
        vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 100ms
    }
}

TaskHandle_t beaconSnifferTaskHandle;

/*
    Starts the beacon sniffing function, then disables it once 'q' is received from the host
*/
void runBeaconSniffer(){
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    xTaskCreate(&beaconSnifferChannelHopper, "beaconSnifferChannelHopper", 4096, NULL, 5, &beaconSnifferTaskHandle);
    while(getchar() != 'q'){ // Wait for 'q' to be sent by host
        vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms delay reduces cpu usage
    }
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    vTaskDelete(beaconSnifferTaskHandle);
}
