#include <Characteristic.h>

Characteristic::Characteristic(NimBLEUUID uuid, uint32_t properties) 
{
  this->UUID = uuid;
  this->Properties = properties;
}

void Characteristic::addSubscription(uint32_t clientID) {
  if (!this->isSubscribed(clientID)) {
    this->subscriptions.push_back(clientID);    
  }
}

void Characteristic::removeSubscription(uint32_t clientID) {
  if (!this->isSubscribed(clientID)) {
    return;
  }
  std::vector<uint32_t> filteredSubscriptions;
  for(uint32_t subscription : this->subscriptions) {
    if (subscription != clientID) {
      filteredSubscriptions.push_back(subscription);
    }
  }
  this->subscriptions = filteredSubscriptions;
}

bool Characteristic::isSubscribed(uint32_t clientID) {
  for(uint32_t subscription : this->subscriptions) {
    if (subscription == clientID) {
      return true;
      break;
    }
  }
  return false;
}

std::vector<uint32_t> Characteristic::getSubscriptions() {
  return this->subscriptions;
}

