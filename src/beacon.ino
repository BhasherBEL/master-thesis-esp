class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        JsonDocument doc;
        doc["rssi"] = advertisedDevice.getRSSI();
        doc["mac"] = advertisedDevice.getAddress().toString().c_str();
        doc["timestamp"] = time(nullptr);
        
        bool isIBeacon = false;
        if (advertisedDevice.haveManufacturerData()) {
            std::string strManufacturerData = advertisedDevice.getManufacturerData();
            uint8_t cManufacturerData[100];
            strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

            if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00) {
                BLEBeacon oBeacon = BLEBeacon();
                oBeacon.setData(strManufacturerData);
                isIBeacon = true;
                
                doc["uuid"] = oBeacon.getProximityUUID().toString().c_str();
                doc["major"] = oBeacon.getMajor();
                doc["minor"] = oBeacon.getMinor();
            } 
        } 

        if(!isIBeacon) {
            return;
        }

        if (advertisedDevice.haveName()) {
            doc["name"] = advertisedDevice.getName().c_str();
        }

        char jsonBuffer[200];
        serializeJson(doc, jsonBuffer);
        
        if (mqttClient.connected()) {
            char topic[100];
            if (isIBeacon && strcmp(doc["uuid"], BEACON_UUID) == 0) {
                snprintf(topic, sizeof(topic), "ibeacon/beacons/%s/%s", 
                    advertisedDevice.getAddress().toString().c_str(),
                    fullUUID);
            } else {
                snprintf(topic, sizeof(topic), "ibeacon/others/%s/%s", 
                    advertisedDevice.getAddress().toString().c_str(),
                    fullUUID);
            }
            if(mqttClient.publish(topic, jsonBuffer)) {
                Serial.print("+");
            } else {
                Serial.print("-");
            }
        } else {
            Serial.print("-");
        }
    }
};

void setupBeacon() {
    BLEDevice::init("ESP32 as iBeacon");

    Serial.println("Starting BLE work!");
    Serial.print("MAC Address: ");
    Serial.println(ESP.getEfuseMac());
    Serial.print("Minor: ");
    Serial.println(beacon->minor);

    BLEServer *pServer = BLEDevice::createServer(); 
    pAdvertising = BLEDevice::getAdvertising();
    BLEDevice::startAdvertising();
    setBeacon();
    pAdvertising->start();
    Serial.println("Advertizing started...");

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    Serial.println("Scanning started...");
}

void setBeacon() {
    BLEBeacon oBeacon = BLEBeacon();
    oBeacon.setManufacturerId(0x4C00); 
    BLEUUID bleUUID = BLEUUID(BEACON_UUID);
    bleUUID = bleUUID.to128();
    oBeacon.setProximityUUID(BLEUUID(bleUUID.getNative()->uuid.uuid128, 16, true));
    oBeacon.setMajor(BEACON_MAJOR);
    oBeacon.setMinor(beacon->minor);
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    oAdvertisementData.setFlags(0x04); 
    std::string strServiceData = "";
    strServiceData += (char)26;
    strServiceData += (char)0xFF;
    strServiceData += oBeacon.getData();
    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
    pAdvertising->setScanResponse(true);
}


