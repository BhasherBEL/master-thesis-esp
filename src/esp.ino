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

#define LED 2

#define WIFI_TIMEOUT 10000
#define WIFI_RECOVER_TIME_MS 500

bool isWiFiConnected = false;

#define BEACON_UUID "4bfde2c7-e489-47a9-965e-484dae07e8dd"
#define BEACON_MAJOR 100
//#define BEACON_MINOR 49494  // red
//#define BEACON_MINOR 2  // blue
#define BEACON_MINOR 3  // green
#define BEACON_SCAN_TIME 5
#define BEACON_STATUS_INTERVAL 10

BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;
unsigned long lastStatusPublish = 0;
char fullUUID[100];

#define MQTT_RECONNECT_DELAY 5000
#define MQTT_MAX_ATTEMPTS 10
#define MQTT_PACKET_SIZE 512

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

char mqttClientId[20];

#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEOUT 10000

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

void setBeacon() {
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00); 
	BLEUUID bleUUID = BLEUUID(BEACON_UUID) ;
	bleUUID = bleUUID.to128();
	oBeacon.setProximityUUID(BLEUUID(bleUUID.getNative()->uuid.uuid128, 16, true));
  oBeacon.setMajor(BEACON_MAJOR);
  oBeacon.setMinor(BEACON_MINOR);
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

void blink(int high, int low) {
	digitalWrite(LED, HIGH);
	delay(high);
	digitalWrite(LED, LOW);
	delay(low);
}

void blinkN(int high, int low, int n) {
	for(int i = 0; i < n; i++) {
		blink(high, low);
	}
}

void blinkSym(int duration) {
	blink(duration, duration);
}

void blinkSymN(int duration, int n) {
	blinkN(duration, duration, n);
}

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

      // Check if it's an iBeacon (Apple's manufacturer ID and iBeacon data length)
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
				// Our iBeacon
				snprintf(topic, sizeof(topic), "ibeacon/beacons/%s/%s", 
					advertisedDevice.getAddress().toString().c_str(),
					fullUUID);
			} else {
				// Any other device (including other iBeacons)
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
	snprintf(mqttClientId, sizeof(mqttClientId), "ESP32_%d", BEACON_MINOR);
	snprintf(fullUUID, sizeof(fullUUID), "%s_%d_%d", BEACON_UUID, BEACON_MAJOR, BEACON_MINOR);
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
	doc["name"] = String("ESP32-") + String(BEACON_MINOR);
  doc["uuid"] = BEACON_UUID;
  doc["major"] = BEACON_MAJOR;
  doc["minor"] = BEACON_MINOR;
	doc["txPower"] = -59;
	doc["X"] = 38.5;
	doc["Y"] = 0.0;
	doc["am"] = 1.0;
	// (0,0), (21,8), (38.5,0)
  
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

void setup() {
	Serial.begin(115200);

	BLEDevice::init("ESP32 as iBeacon");

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

	pinMode(LED, OUTPUT);

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
