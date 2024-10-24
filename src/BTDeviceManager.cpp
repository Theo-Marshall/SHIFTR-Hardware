#include <BTAdvertisedDeviceCallbacks.h>
#include <BTClientCallbacks.h>
#include <BTDeviceManager.h>

std::vector<NimBLEUUID> BTDeviceManager::remoteDeviceFilterUUIDs{};
std::vector<NimBLEAdvertisedDevice *> BTDeviceManager::scannedDevices{};

bool BTDeviceManager::start() {
  if (serviceManager == nullptr) {
    return false;
  }
  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE);
  NimBLEDevice::setScanDuplicateCacheSize(200);
  NimBLEDevice::init(BTDeviceManager::deviceName);
  NimBLEScan *nimBLEScanner = NimBLEDevice::getScan();
  if (nimBLEScanner == nullptr) {
    return false;
  }
  nimBLEScanner->setAdvertisedDeviceCallbacks(new BTAdvertisedDeviceCallbacks(), false);
  nimBLEScanner->setActiveScan(true);
  nimBLEScanner->setInterval(97);
  nimBLEScanner->setWindow(37);
  nimBLEScanner->setMaxResults(0);
  nimBLEScanner->start(0, false);
  return true;
}

bool BTDeviceManager::stop() {
  BTDeviceManager::connected = false;
  if (NimBLEDevice::getScan()->isScanning()) {
    NimBLEDevice::getScan()->stop();
  }
  NimBLEDevice::deinit(true);
  return true;
}

bool BTDeviceManager::isConnected() {
  return BTDeviceManager::connected;
}

void BTDeviceManager::setDeviceName(const std::string &deviceName) {
  BTDeviceManager::deviceName = deviceName;
}

void BTDeviceManager::setRemoteDeviceName(const std::string &remoteDeviceName) {
  BTDeviceManager::remoteDeviceName = remoteDeviceName;
  if (NimBLEDevice::getScan() != nullptr) {
    if (!NimBLEDevice::getScan()->isScanning()) {
      NimBLEDevice::getScan()->start(0, false);
    }
  }
}

void BTDeviceManager::setServiceManager(ServiceManager *serviceManager) {
  BTDeviceManager::serviceManager = serviceManager;
}

void BTDeviceManager::addRemoteDeviceFilter(NimBLEUUID serviceUUID) {
  BTDeviceManager::remoteDeviceFilterUUIDs.push_back(serviceUUID);
}

NimBLEAdvertisedDevice* BTDeviceManager::getRemoteDevice() {
  for (NimBLEAdvertisedDevice *scannedDevice : BTDeviceManager::scannedDevices) {
    if (scannedDevice->getName().rfind(BTDeviceManager::remoteDeviceName, 0) == 0) {
      return scannedDevice;
    }
  }
  return nullptr;
}

bool BTDeviceManager::connectRemoteDevice(NimBLEAdvertisedDevice *remoteDevice) {
  if (remoteDevice != nullptr) {
    BTDeviceManager::nimBLEClient = NimBLEDevice::createClient();
    BTDeviceManager::nimBLEClient->setClientCallbacks(new BTClientCallbacks, false);
    BTDeviceManager::nimBLEClient->setConnectTimeout(10);
    return BTDeviceManager::nimBLEClient->connect(remoteDevice->getAddress(), false);
  }
  return false;
}
