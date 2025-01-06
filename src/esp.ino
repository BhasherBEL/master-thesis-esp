#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "credentials.h"

#define LED 2

#define WIFI_TIMEOUT 10000
#define WIFI_RECOVER_TIME_MS 500

bool isWiFiConnected = false;

#define BEACON_UUID "4bfde2c7-e489-47a9-965e-484dae07e8dd"
#define BEACON_MAJOR 100
//#define BEACON_MINOR 49494  // red
//#define BEACON_MINOR 49495  // blue
#define BEACON_MINOR 49496  // green

BLEAdvertising *pAdvertising;

#define MQTT_CLIENT_ID "ESP32_49496"
#define MQTT_RECONNECT_DELAY 5000
#define MQTT_MAX_ATTEMPTS 10

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

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
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
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


void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
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
    Serial.println("\nFailed to connect to WiFi!");
    isWiFiConnected = false;
		blinkSymN(200, 2);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming messages here
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
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");

	for(int attempt=0; attempt<MQTT_MAX_ATTEMPTS; attempt++){
		unsigned long startAttemptTime = millis();

		if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
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

void setup() {
	Serial.begin(115200);

	BLEDevice::init("ESP32 as iBeacon");

	BLEServer *pServer = BLEDevice::createServer(); 
	
	pAdvertising = BLEDevice::getAdvertising();
	
  BLEDevice::startAdvertising();

  setBeacon();

  pAdvertising->start();

  Serial.println("Advertizing started...");

	pinMode(LED, OUTPUT);

	connectToWiFi();
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
		blink(100, 4900);
  }
}
