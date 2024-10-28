#include <Service.h>

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
  this->getCharacteristic(characteristic.UUID)->subscribeOnSubscriptionChanged(this->handleCharacteristicOnSubscriptionChanged());
}

bool Service::isAdvertised() {
  return this->Advertise;
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

void Service::subscribeOnSubscriptionChanged(void (*onSubscriptionChangedCallback)(Service*, Characteristic*, bool)) {
  this->onSubscriptionChangedCallbacks.push_back(onSubscriptionChangedCallback);
}

void Service::handleCharacteristicOnSubscriptionChanged(Characteristic* characteristic, bool removed) {
  for (size_t callbackIndex = 0; callbackIndex < this->onSubscriptionChangedCallbacks.size(); callbackIndex++)
  {
    this->onSubscriptionChangedCallbacks.at(callbackIndex)(this, characteristic, removed);
  }
}
