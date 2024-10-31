#ifndef BTDEVICEMANAGER_H
#define BTDEVICEMANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ServiceManager.h>
#include <arduino-timer.h>

class BTDeviceManager {
 public:
  static bool start();
  static void stop();
  static void update();
  static void setLocalDeviceName(const std::string localDeviceName);
  static void setRemoteDeviceNameFilter(const std::string remoteDeviceNameFilter);
  static void setServiceManager(ServiceManager* serviceManager);
  static void addRemoteDeviceFilter(NimBLEUUID serviceUUID);
  static bool isConnected();
  static bool isStarted();
  static std::string getConnecedDeviceName();
  static bool writeBLECharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID, std::vector<uint8_t> data);
  static std::vector<uint8_t> readBLECharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID);


 private:
  friend class BTAdvertisedDeviceCallbacks;
  friend class BTClientCallbacks;
  static std::string deviceName;
  static std::string remoteDeviceName;
  static std::string connectedDeviceName;
  static bool connected;
  static bool started;
  static ServiceManager* serviceManager;
  static NimBLEClient* nimBLEClient;
  static std::vector<NimBLEUUID> remoteDeviceFilterUUIDs;
  static std::vector<NimBLEAdvertisedDevice> scannedDevices;
  static Timer<> scanTimer;
  static Timer<> connectTimer;
  static NimBLEAdvertisedDevice* getRemoteDevice();
  static bool connectRemoteDevice(NimBLEAdvertisedDevice* remoteDevice);
  static void onScanEnd(NimBLEScanResults scanResults);
  static void startScan();
  static void stopScan();
  static bool doScan(void* argument);
  static bool doConnect(void* argument);
  static uint32_t getProperties(NimBLERemoteCharacteristic* remoteCharacteristic);
};

#endif