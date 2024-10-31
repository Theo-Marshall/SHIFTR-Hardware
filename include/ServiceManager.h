#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <Service.h>
#include <ServiceManagerCallbacks.h>

class ServiceManager {
 public:
  friend class BTDeviceManager;
  friend class ServiceManagerCharacteristicCallbacks;
  ServiceManager();
  std::vector<Service>* getServices();
  void addService(Service service);
  Service* getService(NimBLEUUID serviceUUID);
  Service* getServiceByCharacteristic(NimBLEUUID characteristicUUID);
  Characteristic* getCharacteristic(NimBLEUUID characteristicUUID);
  void subscribeCallbacks(ServiceManagerCallbacks* callbacks);
 private:
  std::vector<Service> Services;
  std::vector<ServiceManagerCallbacks*> serviceManagerCallbacks;
};

#endif
