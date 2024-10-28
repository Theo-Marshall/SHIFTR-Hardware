#include <BTAdvertisedDeviceCallbacks.h>
#include <BTClientCallbacks.h>
#include <BTDeviceManager.h>
#include <Config.h>
#include <arduino-timer.h>

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

bool BTDeviceManager::start() {
  if (!started) {
    scannedDevices.clear();
    if (serviceManager == nullptr) {
      return false;
    }
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

void BTDeviceManager::addRemoteDeviceFilter(NimBLEUUID serviceUUID) {
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
        serviceManager->addService(Service((*remoteService)->getUUID(), isAdvertising, false));
        service = serviceManager->getService((*remoteService)->getUUID());
      }
      service->Advertise = isAdvertising;
      std::vector<NimBLERemoteCharacteristic*>* remoteCharacteristics = (*remoteService)->getCharacteristics(true);
      for (auto remoteCharacterisic = remoteCharacteristics->begin(); remoteCharacterisic != remoteCharacteristics->end(); remoteCharacterisic++) {
        log_i("Found characteristic %s", (*remoteCharacterisic)->getUUID().to128().toString().c_str());
        Characteristic* characteristic = service->getCharacteristic((*remoteCharacterisic)->getUUID());
        uint32_t properties = 0;
        if (characteristic == nullptr) {
          service->addCharacteristic(Characteristic((*remoteCharacterisic)->getUUID(), getProperties((*remoteCharacterisic))));
          characteristic = service->getCharacteristic((*remoteCharacterisic)->getUUID());
        }
        characteristic->Properties = getProperties((*remoteCharacterisic));
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
