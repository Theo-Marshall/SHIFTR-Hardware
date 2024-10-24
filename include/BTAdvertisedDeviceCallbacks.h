#ifndef BTADVERTISEDDEVICECALLBACKS_H
#define BTADVERTISEDDEVICECALLBACKS_H

#include <NimBLEDevice.h>

class BTAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onDiscovered(NimBLEAdvertisedDevice* advertisedDevice);
  void onResult(NimBLEAdvertisedDevice* advertisedDevice);
  void onScanEnd(NimBLEScanResults scanResults);
};

#endif
