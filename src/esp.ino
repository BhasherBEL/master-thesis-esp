#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "BLEScan.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "time.h"

#include "credentials.h"
#include "beacons.h"
#include "config.h"

BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;
unsigned long lastStatusPublish = 0;
char fullUUID[100];
char mqttClientId[20];
const Beacon* beacon;

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

void setup() {
    Serial.begin(115200);

    uint64_t mac = ESP.getEfuseMac();
    beacon = findBeaconByMac(mac);
    
    if (!beacon) {
        Serial.print("Beacon not found for MAC ");
        Serial.println(mac);
    }

    setupBeacon();
    setupWiFi();
    
    while(WiFi.status() != WL_CONNECTED) {
        connectToWiFi();
    }
    connectToNTP();
    setupMqtt();
    connectToMqtt();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        isWiFiConnected = false;
        connectToWiFi();
    } else if (!isWiFiConnected) {
        isWiFiConnected = true;
    } else if (!mqttClient.connected()) {
        connectToMqtt();
    } else {
        mqttClient.loop();
        deviceStatusLoop();

        BLEScanResults foundDevices = pBLEScan->start(BEACON_SCAN_TIME, false);
        pBLEScan->clearResults();

        blink(100, 4900);
    }
}
