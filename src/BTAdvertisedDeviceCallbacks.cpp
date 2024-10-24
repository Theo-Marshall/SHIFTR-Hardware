#include <BTAdvertisedDeviceCallbacks.h>
#include <BTDeviceManager.h>
#include <NimBLEDevice.h>

void BTAdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice* advertisedDevice) {
  bool addDevice = true;
  for (NimBLEUUID uuid : BTDeviceManager::remoteDeviceFilterUUIDs) {
     if (!advertisedDevice->isAdvertisingService(uuid)) {
      addDevice = false;
      break;
    }
    if (!addDevice) {
      break;
    }  
  }
  if (addDevice) {
    log_d("Adding %s to device list", advertisedDevice->getName().c_str());
    BTDeviceManager::scannedDevices.push_back(advertisedDevice);
  }
}

void BTAdvertisedDeviceCallbacks::onScanEnd(NimBLEScanResults scanResults) {
  if (BTDeviceManager::scannedDevices.size() <= 0) {
    NimBLEDevice::getScan()->start(0, nullptr, false);
  }
}
