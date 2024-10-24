#ifndef BTDEVICEMANAGER_H
#define BTDEVICEMANAGER_H

#include <Arduino.h>
#include <ServiceManager.h>
#include <NimBLEDevice.h>

class BTDeviceManager {
   public:
    static bool start();
    static bool stop();
    static void setDeviceName(const std::string &deviceName);
    static void setRemoteDeviceName(const std::string &remoteDeviceName);
    static void setServiceManager(ServiceManager *ServiceManager);
    static void addRemoteDeviceFilter(NimBLEUUID serviceUUID);
    static bool isConnected();
  private:
    friend class BTAdvertisedDeviceCallbacks;
    friend class BTClientCallbacks;
    static std::string &deviceName;
    static std::string &remoteDeviceName;
    static bool connected; 
    static ServiceManager *serviceManager;
    static NimBLEClient *nimBLEClient;
    static std::vector<NimBLEUUID> remoteDeviceFilterUUIDs;
    static std::vector<NimBLEAdvertisedDevice*> scannedDevices;
};

#endif