#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <Service.h>

class ServiceManager {
 public:
  friend class BTDeviceManager;
  ServiceManager();
  std::vector<Service>* getServices();
  void addService(Service service);
  Service* getService(NimBLEUUID serviceUUID);
  Service* getServiceByCharacteristic(NimBLEUUID characteristicUUID);
  Characteristic* getCharacteristic(NimBLEUUID characteristicUUID);
  void subscribeOnServiceAdded(void (*onServiceAddedCallback)(Service*));

 private:
  std::vector<Service> Services;
  std::vector<void(*)(Service* service)> onServiceAddedCallbacks;
};

#endif
