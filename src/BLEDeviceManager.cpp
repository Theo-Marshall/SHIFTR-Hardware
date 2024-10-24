#include <BLEDeviceManager.h>
#include <AdvertisedDeviceCallbacks.h>

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    BLEDeviceManager::connected = true;
  };

  void onDisconnect(NimBLEClient *pClient) {
    BLEDeviceManager::connected = false;
  };
};

bool BLEDeviceManager::start() {
  if (serviceManager == nullptr) {
    return false;
  }
  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE);
  NimBLEDevice::setScanDuplicateCacheSize(200);
  NimBLEDevice::init("");
  NimBLEScan *nimBLEScanner = NimBLEDevice::getScan();
  if (nimBLEScanner == nullptr) {
    return false;
  }
  nimBLEScanner->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks(), false);
  nimBLEScanner->setActiveScan(true);
  nimBLEScanner->setInterval(97);
  nimBLEScanner->setWindow(37);
  nimBLEScanner->setMaxResults(0);
  return true;
}

bool BLEDeviceManager::stop() {
  if (NimBLEDevice::getScan()->isScanning()) {
    NimBLEDevice::getScan()->stop();
  }
  NimBLEDevice::deinit(true);
  connected = false;
  return true;
}

bool BLEDeviceManager::isConnected() {
  return connected;
}

void BLEDeviceManager::setRemoteDeviceName(const std::string &remoteDeviceName) {
  BLEDeviceManager::remoteDeviceName = remoteDeviceName;
}

void BLEDeviceManager::setServiceManager(ServiceManager *serviceManager) {
  BLEDeviceManager::serviceManager = serviceManager;
}

void BLEDeviceManager::addRemoteDeviceFilter(NimBLEUUID serviceUUID) {
  BLEDeviceManager::remoteDeviceFilterUUIDs.push_back(serviceUUID);
}

void BLEDeviceManager::addScannedDevice(NimBLEAdvertisedDevice *scannedDevice) {
  scannedDevices.push_back(scannedDevice);
}
