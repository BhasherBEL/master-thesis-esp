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
    Beacon(110357712822068, 1, 39.0, 9.0, 1.0, "RED"),
    Beacon(44379490571412, 2, 39.0, 9.0, 1.0, "GREEN"),
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
