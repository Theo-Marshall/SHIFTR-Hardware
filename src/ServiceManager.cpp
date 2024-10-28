#include <ServiceManager.h>
#include <Service.h>

ServiceManager::ServiceManager() {}

std::vector<Service>* ServiceManager::getServices()
{
  return &this->Services;
}

void ServiceManager::addService(Service service) {
  this->Services.push_back(service);
  for (size_t callbackIndex = 0; callbackIndex < this->onServiceAddedCallbacks.size(); callbackIndex++)
  {
    this->onServiceAddedCallbacks.at(callbackIndex)(&this->Services.at(this->Services.size()-1));
  }
}

Service* ServiceManager::getService(NimBLEUUID serviceUUID) {
  for (size_t serviceIndex = 0; serviceIndex < this->Services.size(); serviceIndex++)
  {
    if (this->Services.at(serviceIndex).UUID.equals(serviceUUID)) {
      return &this->Services.at(serviceIndex);
      break;
    }
  }
  return nullptr;
}

Service* ServiceManager::getServiceByCharacteristic(NimBLEUUID characteristicUUID) {
  for (size_t serviceIndex = 0; serviceIndex < this->Services.size(); serviceIndex++)
  {
    if (this->Services.at(serviceIndex).getCharacteristic(characteristicUUID) != nullptr) {
      return &this->Services.at(serviceIndex);
      break;
    }
  }
  return nullptr;
}

Characteristic* ServiceManager::getCharacteristic(NimBLEUUID characteristicUUID) {
  for (size_t serviceIndex = 0; serviceIndex < this->Services.size(); serviceIndex++)
  {
    if (this->Services.at(serviceIndex).getCharacteristic(characteristicUUID) != nullptr) {
      return this->Services.at(serviceIndex).getCharacteristic(characteristicUUID);
      break;
    }
  }
  return nullptr;
}

void ServiceManager::subscribeOnServiceAdded(void (*onServiceAddedCallback)(Service*)) {
  this->onServiceAddedCallbacks.push_back(onServiceAddedCallback);
}
