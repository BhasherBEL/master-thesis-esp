#ifndef CONFIG_H
#define CONFIG_H

// Pin Definitions
#define LED 2

// WiFi Configuration
#define WIFI_TIMEOUT 10000
#define WIFI_RECOVER_TIME_MS 500

// Beacon Configuration
#define BEACON_UUID "4bfde2c7-e489-47a9-965e-484dae07e8dd"
#define BEACON_MAJOR 100
#define BEACON_SCAN_TIME 5
#define BEACON_STATUS_INTERVAL 10

// MQTT Configuration
#define MQTT_RECONNECT_DELAY 5000
#define MQTT_MAX_ATTEMPTS 10
#define MQTT_PACKET_SIZE 512

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEOUT 10000

extern bool isWiFiConnected;
extern const char *mqtt_root_ca;

#endif
