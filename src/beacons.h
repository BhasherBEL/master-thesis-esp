#ifndef BEACONS_H
#define BEACONS_H

#include <stdint.h>

class Beacon {
public:
  const uint64_t mac;
  const uint16_t minor;
  const double x;
  const double y;
  const double am;
  const char *name;

  Beacon(uint64_t mac, uint16_t minor, double x, double y, double am,
         const char *name)
      : mac(mac), minor(minor), x(x), y(y), am(am), name(name) {}
};

// Define known beacons
static const Beacon KNOWN_BEACONS[] = {
    Beacon(110357712822068, 1, 7.0, 10.0, 1.0, "ESP-B3"),
    Beacon(44379490571412, 2, 10.0, 2.5, 1.0, "ESP-B2"),
    Beacon(154918466824980, 3, 1.0, 2.5, 1.0, "ESP-B1"),
    Beacon(62387361766848, 4, 1.0, 10, 1.0, "ESP-B4"),
    Beacon(229754010544916, 5, 10.0, 6.0, 1.0, "ESP-B5"),
    Beacon(115073742364096, 6, 4.5, 3.0, 1.0, "ESP-B6"),
};

static const Beacon *findBeaconByMac(uint64_t mac) {
  for (const auto &beacon : KNOWN_BEACONS) {
    if (beacon.mac == mac) {
      return &beacon;
    }
  }
  return nullptr;
}

#endif // BEACONS_H
