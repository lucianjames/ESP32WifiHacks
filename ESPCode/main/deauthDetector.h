void deauthDetectorCallback(void* buf, wifi_promiscuous_pkt_type_t type){
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    if(pkt->payload[0] == 0xC0){
        printf("==BEGIN DEAUTH INFO==");
        for(int i=4; i<16; i++){
            printf("%c", pkt->payload[i]);
        }
        printf("==END DEAUTH INFO==");
    }
}

void deauthDetectorChannelHopper(void* pvParameters) {
    int ch = 1;
    while(1) {
        ch = ch % 13 + 1; // Cycle through channels 1-13
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
        ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&deauthDetectorCallback)); // Need to re-set the callback function after disabling promiscuous mode
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
        vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 100ms
    }
}

TaskHandle_t deauthDetectorTaskHandle;

void runDeauthDetector(){
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&deauthDetectorCallback));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    xTaskCreate(&deauthDetectorChannelHopper, "deauthDetectorChannelHopper", 4096, NULL, 5, &beaconSnifferTaskHandle);
    while(getchar() != 'q'){ // Wait for 'q' to be sent by host
        vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms delay reduces cpu usage
    }
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    vTaskDelete(deauthDetectorTaskHandle);
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}