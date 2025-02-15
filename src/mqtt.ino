const char *mqtt_root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void setupMqtt() {
    espClient.setCACert(mqtt_root_ca);
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(MQTT_PACKET_SIZE);
    snprintf(mqttClientId, sizeof(mqttClientId), "ESP32_%d", beacon->minor);
    snprintf(fullUUID, sizeof(fullUUID), "%s_%d_%d", BEACON_UUID, BEACON_MAJOR, beacon->minor);
}

void connectToMqtt() {
    Serial.println("Connecting to MQTT...");

    for(int attempt=0; attempt<MQTT_MAX_ATTEMPTS; attempt++){
        unsigned long startAttemptTime = millis();

        if (mqttClient.connect(mqttClientId, MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("Connected to MQTT broker!");
            blinkSymN(1000, 2);
            return;
        } 

        blinkSymN(200, 2);

        while (!mqttClient.connected() && millis() - startAttemptTime < MQTT_RECONNECT_DELAY) {
            blinkSym(50);
            Serial.print(".");
        }
    }

    Serial.println("Failed to connect to MQTT after maximum attempts");
    blinkSymN(200, 4);
}

void publishDeviceStatus() {
    JsonDocument doc;
    
    doc["device_id"] = mqttClientId;
    doc["name"] = String("ESP32-") + String(beacon->minor);
    doc["uuid"] = BEACON_UUID;
    doc["major"] = BEACON_MAJOR;
    doc["minor"] = beacon->minor;
    doc["txPower"] = -59;
    doc["X"] = beacon->x;
    doc["Y"] = beacon->y;
    doc["am"] = beacon->am;
    doc["beacon_name"] = beacon->name;
    
    doc["timestamp"] = time(nullptr);
    
    doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["wifi_ssid"] = WIFI_SSID;
    doc["ip_address"] = WiFi.localIP().toString();
    doc["mqtt_connected"] = mqttClient.connected();
    
    doc["uptime_seconds"] = millis() / 1000;
    doc["heap_free"] = ESP.getFreeHeap();
    doc["heap_size"] = ESP.getHeapSize();
    doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
    
    char jsonBuffer[400];
    size_t len = serializeJson(doc, jsonBuffer);

    char topic[100];
    snprintf(topic, sizeof(topic), "ibeacon/devices/%s", fullUUID);
    
    if (mqttClient.publish(topic, (uint8_t*) jsonBuffer, len, true)) {
        Serial.print("*");
    } else {
        Serial.print("-");
        Serial.printf(" (MQTT State: %d)", mqttClient.state());
    }
}

void deviceStatusLoop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastStatusPublish >= BEACON_STATUS_INTERVAL * 1000) {
        publishDeviceStatus();
        lastStatusPublish = currentMillis;
    }
}
