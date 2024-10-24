#ifndef ADVERTISEDDEVICECALLBACKS_H
#define ADVERTISEDDEVICECALLBACKS_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <BLEDeviceManager.h>

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    bool addDevice = true;
    /*
    for (size_t filterIndex = 0; filterIndex < BLEDeviceManager::remoteDeviceFilterUUIDs.size(); filterIndex++) {
      if (!advertisedDevice->isAdvertisingService(BLEDeviceManager::remoteDeviceFilterUUIDs.at(filterIndex))) {
        addDevice = false;
        break;
      }
      if (!addDevice) {
        break;
      }
    }
    */
    if (addDevice) {
      log_d("Adding %s to device list", advertisedDevice->getName().c_str());
      BLEDeviceManager::addScannedDevice(advertisedDevice);
    }
  };
};

#endif
