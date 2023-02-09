#pragma once

void wifiInit(bool promiscuous){
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t ap_config = {
		.ap = {
			.ssid = " ",
			.ssid_len = 0,
			.password = "7fd6v24sg",
			.channel = 11,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.ssid_hidden = 1,
			.max_connection = 4,
			.beacon_interval = 100
		}
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(promiscuous));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

void wifiDeInit(){
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}