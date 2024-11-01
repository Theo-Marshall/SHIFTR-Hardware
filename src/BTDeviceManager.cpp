#include <BTAdvertisedDeviceCallbacks.h>
#include <BTClientCallbacks.h>
#include <BTDeviceManager.h>
#include <Config.h>
#include <arduino-timer.h>
#include <DirConManager.h>

std::vector<NimBLEUUID> BTDeviceManager::remoteDeviceFilterUUIDs{};
std::vector<NimBLEAdvertisedDevice> BTDeviceManager::scannedDevices{};
ServiceManager* BTDeviceManager::serviceManager{};
std::string BTDeviceManager::deviceName;
std::string BTDeviceManager::remoteDeviceName;
std::string BTDeviceManager::connectedDeviceName;
NimBLEClient* BTDeviceManager::nimBLEClient{};
bool BTDeviceManager::connected = false;
bool BTDeviceManager::started = false;
Timer<> BTDeviceManager::scanTimer = timer_create_default();
Timer<> BTDeviceManager::connectTimer = timer_create_default();

class BTDeviceServiceManagerCallbacks : public ServiceManagerCallbacks {
  void onCharacteristicSubscriptionChanged(Characteristic* characteristic, bool removed) {
    if (removed && (characteristic->getSubscriptions().size() == 0)) {
      BTDeviceManager::changeBLENotify(characteristic, true);
    } else {
      if (characteristic->getSubscriptions().size() > 0) {
        BTDeviceManager::changeBLENotify(characteristic, false);
      }
    }
  };
};

void BTDeviceManager::onBLENotify(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  log_d("Notify for characteristic %s, length %d", pBLERemoteCharacteristic->getUUID().toString().c_str(), length);
  DirConManager::notifyDirConCharacteristic(pBLERemoteCharacteristic->getUUID(), pData, length);
}

bool BTDeviceManager::start() {
  if (!started) {
    scannedDevices.clear();
    if (serviceManager == nullptr) {
      return false;
    }
    serviceManager->subscribeCallbacks(new BTDeviceServiceManagerCallbacks);
    NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE);
    NimBLEDevice::setScanDuplicateCacheSize(200);
    NimBLEDevice::init(deviceName);
    started = true;
    startScan();
    return true;
  }
  return false;
}

void BTDeviceManager::stop() {
  connected = false;
  started = false;
  connectTimer.cancel();
  stopScan();
}

void BTDeviceManager::update() {
  scanTimer.tick();
  connectTimer.tick();
  if (connected) {
    if (nimBLEClient != nullptr) {
      if (!nimBLEClient->isConnected()) {
        connected = false;
        connectedDeviceName = "";
        startScan();
      }
    }
  }
}

bool BTDeviceManager::isConnected() {
  return connected;
}

std::string BTDeviceManager::getConnecedDeviceName() {
  return connectedDeviceName;
}

bool BTDeviceManager::isStarted() {
  return started;
}

void BTDeviceManager::setLocalDeviceName(const std::string localDeviceName) {
  deviceName = localDeviceName;
}

void BTDeviceManager::setRemoteDeviceNameFilter(const std::string remoteDeviceNameFilter) {
  remoteDeviceName = remoteDeviceNameFilter;
  if (started) {
    if (NimBLEDevice::getScan() != nullptr) {
      if (!NimBLEDevice::getScan()->isScanning()) {
        startScan();
      } else {
        if (BTDeviceManager::getRemoteDevice() != nullptr) {
          stopScan();
        }
      }
    }
  }
}

void BTDeviceManager::setServiceManager(ServiceManager* serviceManager) {
  BTDeviceManager::serviceManager = serviceManager;
}

void BTDeviceManager::addRemoteDeviceFilter(const NimBLEUUID& serviceUUID) {
  remoteDeviceFilterUUIDs.push_back(serviceUUID);
}

NimBLEAdvertisedDevice* BTDeviceManager::getRemoteDevice() {
  for (size_t deviceIndex = 0; deviceIndex < scannedDevices.size(); deviceIndex++) {
    NimBLEAdvertisedDevice* scannedDevice = &scannedDevices.at(deviceIndex);
    if (scannedDevice->haveName()) {
      if (scannedDevice->getName().rfind(remoteDeviceName, 0) == 0) {
        return &scannedDevices.at(deviceIndex);
      }
    }
  }
  return nullptr;
}

bool BTDeviceManager::connectRemoteDevice(NimBLEAdvertisedDevice* remoteDevice) {
  log_i("Connecting to BLE device %s (%s)...", remoteDevice->getAddress().toString().c_str(), remoteDevice->getName().c_str());
  stopScan();
  nimBLEClient = nullptr;
  nimBLEClient = NimBLEDevice::createClient();
  nimBLEClient->setClientCallbacks(new BTClientCallbacks(), true);
  nimBLEClient->setConnectTimeout(BLE_CONNECT_TIMEOUT);
  if (!nimBLEClient->connect(remoteDevice)) {
    log_e("Connection to BLE device failed, error %d", nimBLEClient->getLastError());
    nimBLEClient->disconnect();
    connectedDeviceName = "";
    return false;
  }
  connected = true;
  connectedDeviceName = remoteDevice->getName();
  if (serviceManager != nullptr) {
    log_d("Iterating through remote BLE services and characteristics...");
    std::vector<NimBLERemoteService*>* remoteServices = nimBLEClient->getServices(true);
    for (auto remoteService = remoteServices->begin(); remoteService != remoteServices->end(); remoteService++) {
      log_i("Found service %s, iterating through characteristics...", (*remoteService)->getUUID().to128().toString().c_str());
      Service* service = serviceManager->getService((*remoteService)->getUUID());
      bool isAdvertising = remoteDevice->isAdvertisingService((*remoteService)->getUUID());
      if (service == nullptr) {
        service = new Service((*remoteService)->getUUID(), isAdvertising, false);
        serviceManager->addService(service);
      }
      service->Advertise = isAdvertising;
      std::vector<NimBLERemoteCharacteristic*>* remoteCharacteristics = (*remoteService)->getCharacteristics(true);
      for (auto remoteCharacterisic = remoteCharacteristics->begin(); remoteCharacterisic != remoteCharacteristics->end(); remoteCharacterisic++) {
        log_i("Found characteristic %s", (*remoteCharacterisic)->getUUID().to128().toString().c_str());
        Characteristic* characteristic = service->getCharacteristic((*remoteCharacterisic)->getUUID());
        uint32_t properties = 0;
        if (characteristic == nullptr) {
          characteristic = new Characteristic((*remoteCharacterisic)->getUUID(), getProperties((*remoteCharacterisic)));
          service->addCharacteristic(characteristic);
        }
        characteristic->setProperties(getProperties((*remoteCharacterisic)));
      }
    }

  } else {
    log_e("Connection to BLE device failed, service manager not available");
    nimBLEClient->disconnect();
    connectedDeviceName = "";
    return false;
  }
  return true;
}

void BTDeviceManager::startScan() {
  if (started) {
    log_d("Deactivating BLE connect timer");
    connectTimer.cancel();
    log_d("Activating BLE scan timer");
    scanTimer.every(BLE_SCAN_INTERVAL, BTDeviceManager::doScan);
  } else {
    log_d("Deactivating BLE scan timer");
    scanTimer.cancel();
  }
}

void BTDeviceManager::stopScan() {
  log_d("Deactivating BLE scan timer");
  scanTimer.cancel();
  NimBLEDevice::getScan()->stop();
}

bool BTDeviceManager::doScan(void* argument) {
  if (started) {
    log_i("Starting BLE scan...");
    if (nimBLEClient != nullptr) {
      if (nimBLEClient->isConnected()) {
        nimBLEClient->disconnect();
        connectedDeviceName = "";
        connected = false;
      }
    }
    NimBLEScan* nimBLEScanner = NimBLEDevice::getScan();
    nimBLEScanner->setAdvertisedDeviceCallbacks(new BTAdvertisedDeviceCallbacks(), false);
    nimBLEScanner->setActiveScan(true);
    nimBLEScanner->setInterval(97);
    nimBLEScanner->setWindow(37);
    nimBLEScanner->setMaxResults(0);
    scannedDevices.clear();
    if (nimBLEScanner->start(0, BTDeviceManager::onScanEnd, false)) {
      log_e("BLE scan started");
      return false;
    }
  }
  log_e("BLE scan start failed");
  return true;
}

void BTDeviceManager::onScanEnd(NimBLEScanResults scanResults) {
  log_i("BLE scan ended with %d devices", scannedDevices.size());
  if (scannedDevices.size() <= 0) {
    if (!connected) {
      startScan();
    }
  } else {
    if (!connected) {
      connectTimer.every(BLE_CONNECT_INTERVAL, BTDeviceManager::doConnect);
    }
  }
}

bool BTDeviceManager::doConnect(void* argument) {
  log_i("Starting BLE connection...");
  if (started) {
    if (connectRemoteDevice(getRemoteDevice())) {
      return false;
    }
  }
  log_e("BLE connection failed: Bluetooth device manager not started");
  return true;
}

uint32_t BTDeviceManager::getProperties(NimBLERemoteCharacteristic* remoteCharacteristic) {
  uint32_t returnValue = 0x00;
  if (remoteCharacteristic->canRead()) {
    returnValue = returnValue | READ;
  }
  if (remoteCharacteristic->canWrite() || remoteCharacteristic->canWriteNoResponse()) {
    returnValue = returnValue | WRITE;
  }
  if (remoteCharacteristic->canIndicate() || remoteCharacteristic->canNotify() || remoteCharacteristic->canBroadcast()) {
    returnValue = returnValue | NOTIFY;
  }
  return returnValue;
}

bool BTDeviceManager::writeBLECharacteristic(const NimBLEUUID& serviceUUID, const NimBLEUUID& characteristicUUID, std::vector<uint8_t>* data) {
  if (!connected) {
    log_e("BLE characteristic write failed: Not connected");
    return false;
  }
  NimBLERemoteService* remoteService = nullptr;
  NimBLERemoteCharacteristic* remoteCharacteristic = nullptr;
  remoteService = nimBLEClient->getService(serviceUUID);
  if (remoteService) {
    remoteCharacteristic = remoteService->getCharacteristic(characteristicUUID);
    if (remoteCharacteristic) {
      if (remoteCharacteristic->canWrite()) {
        if (remoteCharacteristic->writeValue(data)) {
          return true;
        }
      } else {
        log_e("BLE characteristic write failed: Remote characteristic not writable");
      }
    } else {
      log_e("BLE characteristic write failed: Remote characteristic not found");
    }
  } else {
    log_e("BLE characteristic write failed: Remote service not found");
  }
  return false;
}

std::vector<uint8_t> BTDeviceManager::readBLECharacteristic(const NimBLEUUID& serviceUUID, const NimBLEUUID& characteristicUUID) {
  std::vector<uint8_t> returnData;
  if (!connected) {
    log_e("BLE characteristic read failed: Not connected");
    return returnData;
  }
  NimBLERemoteService* remoteService = nullptr;
  NimBLERemoteCharacteristic* remoteCharacteristic = nullptr;
  remoteService = nimBLEClient->getService(serviceUUID);
  if (remoteService) {
    remoteCharacteristic = remoteService->getCharacteristic(characteristicUUID);
    if (remoteCharacteristic) {
      if (remoteCharacteristic->canRead()) {
        returnData = remoteCharacteristic->readValue();
      } else {
        log_e("BLE characteristic read failed: Remote characteristic not readable");
      }
    } else {
      log_e("BLE characteristic read failed: Remote characteristic not found");
    }
  } else {
    log_e("BLE characteristic read failed: Remote service not found");
  }
  return returnData;
}

bool BTDeviceManager::changeBLENotify(Characteristic* characteristic, bool remove) {
  if (!connected) {
    log_e("BLE characteristic change notify failed: Not connected");
    return false;
  }
  NimBLERemoteService* remoteService = nullptr;
  NimBLERemoteCharacteristic* remoteCharacteristic = nullptr;
  remoteService = nimBLEClient->getService(characteristic->getService()->UUID);
  if (remoteService) {
    remoteCharacteristic = remoteService->getCharacteristic(characteristic->UUID);
    if (remoteCharacteristic) {
      if (remoteCharacteristic->canNotify() || remoteCharacteristic->canIndicate()) {
        if (!remove) {
          return remoteCharacteristic->subscribe(true, onBLENotify);
        } else {
          return remoteCharacteristic->unsubscribe();
        }
      } else {
        log_e("BLE characteristic change notify failed: Remote characteristic can't notify nor indicate");
      }
    } else {
      log_e("BLE characteristic change notify failed: Remote characteristic not found");
    }
  } else {
    log_e("BLE characteristic change notify failed: Remote service not found");
  }
  return false;
}
