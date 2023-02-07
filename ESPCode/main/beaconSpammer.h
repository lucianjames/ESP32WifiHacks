 uint8_t beaconFrame_p1[] = {
    0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};
// Source MAC address goes here (6 bytes)
// BSSID goes here (6 bytes) (same as source MAC address)
uint8_t beaconFrame_p2[] = {
    0xc0, 0x05, 0x26, 0x23, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, // Seq num + timestamp
    0x64, 0x00, 0x11, 0x04, 0x00, // Beacon interval (set to 300ms)
}; // Default for beacon interval is 100ms 0x64, 0x00
// SSID length goes here (1 byte)
// SSID goes here (<length> bytes)
uint8_t beaconFrame_p3[] = {
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12,
    0x18, 0x24, 0x32, 0x04, 0x30, 0x48, 0x60, 0x6c, 0x03, 0x01,
};
// Channel goes here (1 byte)
uint8_t beaconFrame_p4[] = {
    0x05, 0x04, 0x00, 0x02, 0x00, 
    0x00, 0x2a, 0x01, 0x04, 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f,
    0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x0c, 0x00, 0x2d, 0x1a, 0x2d, 0x09, 0x17, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x16, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x08,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdd, 0x18, 0x00, 0x50, 0xf2, 0x02, 0x01, 0x01,
    0x01, 0x00, 0x03, 0xa4, 0x00, 0x00, 0x27, 0xa4, 0x00, 0x00, 0x42, 0x43, 0x5e, 0x00, 0x62, 0x32,
    0x2f, 0x00, 0xdd, 0x05, 0x00, 0x16, 0x32, 0x80, 0x00, 0xdd, 0x08, 0x00, 0x50, 0xf2, 0x11, 0x02,
    0x00, 0x00, 0x00,
};

struct apInfo{
    char* ssid;
    int ssidLen;
    uint8_t channel;
    uint8_t bssid[6];
};

/*
    This declaration gives access to the esp_wifi_80211_tx function,
    which gives the ability to send raw arbitrary 802.11 frames.
*/
esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

// These variables are global so they can be freed from wherever
struct apInfo* APs;
char** ssids;

void beaconSpammer(){
    // Read the full command from the host
    // This will give a list of SSIDs to spam
    int cmdLen = 128;
    char* cmd = (char*)malloc(cmdLen * sizeof(char));
    int pos = 0;
    int c = 0;
    while(c != '\n'){
        do{
            c=getchar();
        }while(c==-1);
        cmd[pos] = c;
        pos++;
        if(pos >= cmdLen-1){
            cmdLen += 128;
            cmd = (char*)realloc(cmd, cmdLen * sizeof(char));
        }
    }
    cmd[pos-1] = '\0'; // Remove the newline character

    // Parse the command
    int ssidCount = 0;
    int ssidArrLen = 10;
    ssids = malloc(ssidArrLen * sizeof(char*));
    char delim[] = {0x03, 0x00};
    char* ssid = strtok(cmd, delim);
    while(ssid != NULL){
        ssids[ssidCount] = (char*)malloc((strlen(ssid)+1) * sizeof(char));
        strcpy(ssids[ssidCount], ssid);
        ssidCount++;
        if(ssidCount >= ssidArrLen){
            ssidArrLen += 10;
            ssids = (char**)realloc(ssids, ssidArrLen * sizeof(char*));
        }
        ssid = strtok(NULL, delim);
    }
    free(cmd);

    // Create an array of apInfo structs, one for each SSID
    APs = malloc(sizeof(struct apInfo) * ssidCount);
    for(int i=0; i<ssidCount; i++){
        APs[i].ssid = ssids[i];
        APs[i].ssidLen = strlen(ssids[i]);
        // Generate a random BSSID
        for(int j=0; j<6; j++){
            APs[i].bssid[j] = rand() % 256;
        }
        APs[i].channel = 11; // The ap_config thing is set to 11, so ill just put them all on 11 for now
    }

    // Set up the WIFI for sending out beacon frames
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
			.channel = 11,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.ssid_hidden = 1,
			.max_connection = 4,
			.beacon_interval = 60000
		}
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    // Send the beacon frame every 100ms
    int apIdx = 0;
    while(1){
        // Assemble a beacon frame for the current AP
        uint8_t* frame = (uint8_t*)malloc(256*sizeof(uint8_t));
        uint8_t* p = frame;
        memcpy(p, beaconFrame_p1, sizeof(beaconFrame_p1));
        p += sizeof(beaconFrame_p1);
        memcpy(p, APs[apIdx].bssid, 6);
        p += 6;
        memcpy(p, APs[apIdx].bssid, 6);
        p += 6;
        memcpy(p, beaconFrame_p2, sizeof(beaconFrame_p2));
        p += sizeof(beaconFrame_p2);
        *p = APs[apIdx].ssidLen;
        p++;
        //memcpy(p, APs[apIdx].ssid, APs[apIdx].ssidLen);
        // The above line doesnt work, so heres the below line instead:
        for(int i=0; i<APs[apIdx].ssidLen; i++){
            *p = APs[apIdx].ssid[i];
            p++;
        }
        memcpy(p, beaconFrame_p3, sizeof(beaconFrame_p3));
        p += sizeof(beaconFrame_p3);
        *p = APs[apIdx].channel;
        p++;
        memcpy(p, beaconFrame_p4, sizeof(beaconFrame_p4));
        p += sizeof(beaconFrame_p4);

        esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, p-frame, false);
        free(frame); // Free the frame memory
        if(err != ESP_OK){
            printf("Error sending beacon frame: %d\r\n", err);
        }
        // <10ms delay in the loop will cause esp_wifi_80211_tx to fail. Target delay is 100ms between beacons on each AP.
        vTaskDelay(10 / portTICK_PERIOD_MS);
        apIdx = (apIdx+1)%ssidCount;
    }
}

TaskHandle_t beaconSpammerTaskHandle;

void runBeaconSpammer(){
    xTaskCreate(beaconSpammer, "beaconSpammer", 4096*2, NULL, 5, &beaconSpammerTaskHandle);
    while(getchar() != 'q'){ // Wait for 'q' to be sent by host
        vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms delay reduces cpu usage
    }
    vTaskDelete(beaconSpammerTaskHandle);
    // Free the memory malloc'd by beaconSpammer
    free(ssids);
    free(APs);
}