#include <ServiceManager.h>
#include <Service.h>

class ServiceManagerCharacteristicCallbacks : public CharacteristicCallbacks {
  void onSubscriptionChanged(Characteristic* characteristic, bool removed) {
    ServiceManager* serviceManager = characteristic->getService()->getServiceManager();
    for (size_t callbackIndex = 0; callbackIndex < serviceManager->serviceManagerCallbacks.size(); callbackIndex++)
    {
      serviceManager->serviceManagerCallbacks.at(callbackIndex)->onCharacteristicSubscriptionChanged(characteristic, removed);
    }
    
  };
};

ServiceManager::ServiceManager() {}

std::vector<Service>* ServiceManager::getServices()
{
  return &this->Services;
}

void ServiceManager::addService(Service service) {
  this->Services.push_back(service);
  Service* addedService = &this->Services.at(this->Services.size()-1);
  addedService->serviceManager = this;
  addedService->subscribeCallbacks(new ServiceManagerCharacteristicCallbacks);
  for (size_t callbackIndex = 0; callbackIndex < this->serviceManagerCallbacks.size(); callbackIndex++)
  {
    this->serviceManagerCallbacks.at(callbackIndex)->onServiceAdded(addedService);
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

void ServiceManager::subscribeCallbacks(ServiceManagerCallbacks* callbacks) {
  this->serviceManagerCallbacks.push_back(callbacks);
}
