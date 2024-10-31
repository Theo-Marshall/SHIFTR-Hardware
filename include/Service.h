#ifndef SERVICE_H
#define SERVICE_H

#include <Characteristic.h>
#include <NimBLEUUID.h>
#include <vector>
#include <CharacteristicCallbacks.h>

class ServiceManager;

class Service {
 public:
  friend class Characteristic;
  friend class ServiceCharacteristicCallbacks;
  friend class ServiceManager;
  Service(NimBLEUUID uuid);
  Service(NimBLEUUID uuid, bool advertise, bool internal);
  void addCharacteristic(Characteristic characteristic);
  std::vector<Characteristic>* getCharacteristics();
  bool isAdvertised();
  bool isInternal();
  Characteristic* getCharacteristic(NimBLEUUID characteristicUUID);
  ServiceManager* getServiceManager();
  void subscribeCallbacks(CharacteristicCallbacks* callbacks);
  NimBLEUUID UUID;
  std::vector<Characteristic> Characteristics;
  bool Advertise = false;
  bool Internal = false;

 private:
  ServiceManager* serviceManager;
  std::vector<CharacteristicCallbacks*> characteristicCallbacks;

};

#endif
