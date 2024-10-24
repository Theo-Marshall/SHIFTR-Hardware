#ifndef BLEDEVICEMANAGER_H
#define BLEDEVICEMANAGER_H

#include <Arduino.h>
#include <ServiceManager.h>
#include <NimBLEDevice.h>

class BLEDeviceManager {
   public:
    static bool start();
    static bool stop();
    static void setRemoteDeviceName(const std::string &remoteDeviceName);
    static void setServiceManager(ServiceManager *ServiceManager);
    static void addRemoteDeviceFilter(NimBLEUUID serviceUUID);
    static void addScannedDevice(NimBLEAdvertisedDevice *scannedDevice);
    static bool isConnected();
    friend class AdvertisedDeviceCallbacks;
    friend class ClientCallbacks;
  private:
    static std::string &remoteDeviceName;
    static bool connected; 
    static ServiceManager *serviceManager;
    static NimBLEClient *nimBLEClient;
    static std::vector<NimBLEUUID> remoteDeviceFilterUUIDs;
    static std::vector<NimBLEAdvertisedDevice *> scannedDevices;
};

#endif