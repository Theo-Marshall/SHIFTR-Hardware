#include <Service.h>

class ServiceCharacteristicCallbacks : public CharacteristicCallbacks {
  void onSubscriptionChanged(Characteristic* characteristic, bool removed) {
    Service* service = characteristic->getService();
    for (size_t callbackIndex = 0; callbackIndex < service->characteristicCallbacks.size(); callbackIndex++)
    {
      service->characteristicCallbacks.at(callbackIndex)->onSubscriptionChanged(characteristic, removed);
    }
  };
};

Service::Service(NimBLEUUID uuid) {
  this->UUID = uuid;
}

Service::Service(NimBLEUUID uuid, bool advertise, bool internal) {
  this->UUID = uuid;
  this->Advertise = advertise;
  this->Internal = internal;
}

void Service::addCharacteristic(Characteristic characteristic) {
  this->Characteristics.push_back(characteristic);
  Characteristic* addedCharacteristic = &this->Characteristics.at(this->Characteristics.size()-1);
  addedCharacteristic->service = this;
  addedCharacteristic->subscribeCallbacks(new ServiceCharacteristicCallbacks);
}

bool Service::isAdvertised() {
  return this->Advertise;
}

bool Service::isInternal() {
  return this->Internal;
}

Characteristic* Service::getCharacteristic(NimBLEUUID characteristicUUID) {
  for (size_t characteristicIndex = 0; characteristicIndex < this->Characteristics.size(); characteristicIndex++)
  {
    if (this->Characteristics.at(characteristicIndex).UUID.equals(characteristicUUID)) {
      return &this->Characteristics.at(characteristicIndex);
      break;
    }
  }
  return nullptr;
}

std::vector<Characteristic>* Service::getCharacteristics() {
  return &this->Characteristics;
}

ServiceManager* Service::getServiceManager() {
  return this->serviceManager;
}

void Service::subscribeCallbacks(CharacteristicCallbacks* callbacks) {
  this->characteristicCallbacks.push_back(callbacks);
}