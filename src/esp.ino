#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include <WiFi.h>

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
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    isWiFiConnected = false;
    connectToWiFi();
  } else if (!isWiFiConnected) {
    isWiFiConnected = true;
  } else {
		blink(100, 4900);
  }
}
