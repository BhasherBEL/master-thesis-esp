#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"

#define LED 2
#define BEACON_UUID "4bfde2c7-e489-47a9-965e-484dae07e8dd"
#define BEACON_MAJOR 100
//#define BEACON_MINOR 49494
//#define BEACON_MINOR 49495
#define BEACON_MINOR 49496

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

void initBeacon() {
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
}

void loop() {
  delay(4900);
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
}
