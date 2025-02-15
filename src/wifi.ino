bool isWiFiConnected = false;

void setupWiFi() {
    pinMode(LED, OUTPUT);
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi (");
    Serial.print(WIFI_SSID);
    Serial.println(")...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && 
           millis() - startAttemptTime < WIFI_TIMEOUT) {
        blinkSym(50);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("\nConnected! IP address: ");
        Serial.println(WiFi.localIP());
        isWiFiConnected = true;
        blinkSymN(1000, 2);
    } else {
        Serial.println("\nFailed to connect to WiFi! Status: " + String(WiFi.status()));
        isWiFiConnected = false;
        blinkSymN(200, 2);
    }
}

void connectToNTP() {
    Serial.println("Connecting to NTP server...");
    configTime(0, 0, NTP_SERVER);

    unsigned long startAttemptTime = millis();
    
    time_t now = time(nullptr);
    while (now < 24 * 3600 && millis() - startAttemptTime < NTP_TIMEOUT) {
        blinkSym(50);
        Serial.print(".");
        now = time(nullptr);
    }

    if(now < 24 * 3600) {
        Serial.println("\nFailed to connect to NTP server");
        blinkSymN(200, 2);
    } else {
        Serial.println("\nTime synchronized with NTP server");
        blinkSymN(1000, 2);
    }
}
