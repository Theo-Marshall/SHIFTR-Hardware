#include <BTAdvertisedDeviceCallbacks.h>
#include <BTClientCallbacks.h>
#include <BTDeviceManager.h>
#include <Config.h>
#include <arduino-timer.h>
#include <DirConManager.h>
#include <Utils.h>

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
    if (!characteristic->getService()->isInternal()) {
      if (removed && (characteristic->getSubscriptions().size() == 0)) {
        BTDeviceManager::changeBLENotify(characteristic, true);
      } else {
        if (characteristic->getSubscriptions().size() > 0) {
          BTDeviceManager::changeBLENotify(characteristic, false);
        }
      }
    }
  };
};

void BTDeviceManager::onBLENotify(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  //log_i("BLE forward notification characteristic %s with data %s", pBLERemoteCharacteristic->getUUID().toString().c_str(), Utils::getHexString(pData, length).c_str());
  DirConManager::notifyDirConCharacteristic(pBLERemoteCharacteristic->getUUID(), pData, length);
}

std::vector<NimBLEAdvertisedDevice>* BTDeviceManager::getScannedDevices() {
  return &scannedDevices;
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
  if (remoteDeviceName != "") {
    for (size_t deviceIndex = 0; deviceIndex < scannedDevices.size(); deviceIndex++) {
      NimBLEAdvertisedDevice* scannedDevice = &scannedDevices.at(deviceIndex);
      if (scannedDevice->haveName()) {
        if (scannedDevice->getName().rfind(remoteDeviceName, 0) == 0) {
          return &scannedDevices.at(deviceIndex);
        }
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
      // little hack for service advertising that gets truncated
      if (!isAdvertising) {
        if ((*remoteService)->getUUID().equals(NimBLEUUID(TACX_FEC_PRIMARY_SERVICE_UUID))) {
          isAdvertising = true;
        }
      }
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
    NimBLEScan* nimBLEScanner = NimBLEDevice::getScan();
    if (!nimBLEScanner->isScanning()) {
      log_i("Starting BLE scan...");
      if (nimBLEClient != nullptr) {
        if (nimBLEClient->isConnected()) {
          nimBLEClient->disconnect();
          connectedDeviceName = "";
          connected = false;
        }
      }
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
    } else {
        log_e("BLE scan already running");
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
        // value needs to be copied to this scope as it will be handled asynchronously by another core and NimBLE will only write garbage
        std::vector<uint8_t> valueData;
        for (auto currentByte = data->begin(); currentByte != data->end(); currentByte++) {
          valueData.push_back(*currentByte);
        }
        if (remoteCharacteristic->writeValue(valueData)) {
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
        log_e("BLE characteristic change notify failed: Remote characteristic %s can't notify nor indicate", characteristic->UUID.to128().toString().c_str());
      }
    } else {
      log_e("BLE characteristic change notify failed: Remote characteristic %s not found", characteristic->UUID.to128().toString().c_str());
    }
  } else {
    log_e("BLE characteristic change notify failed: Remote service for characteristic %s not found", characteristic->UUID.to128().toString().c_str());
  }
  return false;
}

// BLE write characteristic 6e40fec3-b5a3-f393-e0a9-e50e24dcca9e with data [A4 09 4E 05 31 FF FF FF FF FF B8 01 40] =110W
// BLE write characteristic 6e40fec3-b5a3-f393-e0a9-e50e24dcca9e with data [A4 09 4E 05 31 FF FF FF FF FF CC 01 54] =115W

bool BTDeviceManager::writeFECTargetPower(uint16_t targetPower) {
  std::vector<uint8_t> fecData;
  fecData.push_back(0xA4);  // SYNC
  fecData.push_back(0x09);  // MSG_LEN
  fecData.push_back(0x4E);  // MSG_ID
  fecData.push_back(0x05);  // CONTENT_START
  fecData.push_back(0x31);  // PAGE 49 (0x31)
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back((uint8_t)targetPower);
  fecData.push_back((uint8_t)(targetPower >> 8));
  fecData.push_back(getFECChecksum(&fecData));  // CHECKSUM
  return writeBLECharacteristic(NimBLEUUID(TACX_FEC_PRIMARY_SERVICE_UUID), NimBLEUUID(TACX_FEC_WRITE_CHARACTERISTIC_UUID), &fecData);
}

// BLE write characteristic 6e40fec3-b5a3-f393-e0a9-e50e24dcca9e with data [A4 09 4E 05 33 FF FF FF FF 20 4E FF F9]

bool BTDeviceManager::writeFECTrackResistance(uint16_t grade, uint8_t rollingResistance) {
  std::vector<uint8_t> fecData;
  fecData.push_back(0xA4);  // SYNC
  fecData.push_back(0x09);  // MSG_LEN
  fecData.push_back(0x4E);  // MSG_ID
  fecData.push_back(0x05);  // CONTENT_START
  fecData.push_back(0x33);  // PAGE 51 (0x33)
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back((uint8_t)grade);
  fecData.push_back((uint8_t)(grade >> 8));
  fecData.push_back(rollingResistance);
  fecData.push_back(getFECChecksum(&fecData));  // CHECKSUM
  return writeBLECharacteristic(NimBLEUUID(TACX_FEC_PRIMARY_SERVICE_UUID), NimBLEUUID(TACX_FEC_WRITE_CHARACTERISTIC_UUID), &fecData);
}

uint8_t BTDeviceManager::getFECChecksum(std::vector<uint8_t>* fecData) {
  uint8_t checksum = 0;
  if (fecData->size() > 0) {
    checksum = fecData->at(0);
    if (fecData->size() > 1) {
      for (size_t checksumIndex = 1; checksumIndex < fecData->size(); checksumIndex++) {
        checksum = (checksum ^ fecData->at(checksumIndex));
      }
    }
  }
  return checksum;
}
