#include <Characteristic.h>

Characteristic::Characteristic(NimBLEUUID uuid, uint32_t properties) 
{
  this->UUID = uuid;
  this->Properties = properties;
}

void Characteristic::addSubscription(uint32_t clientID) {
  if (!this->isSubscribed(clientID)) {
    this->subscriptions.push_back(clientID);    
    for (size_t callbackIndex = 0; callbackIndex < this->onSubscriptionChangedCallbacks.size(); callbackIndex++)
    {
      this->onSubscriptionChangedCallbacks.at(callbackIndex)(this, false);
    }
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
  for (size_t callbackIndex = 0; callbackIndex < this->onSubscriptionChangedCallbacks.size(); callbackIndex++)
  {
    this->onSubscriptionChangedCallbacks.at(callbackIndex)(this, true);
  }
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

void Characteristic::subscribeOnSubscriptionChanged(void (*onSubscriptionChangedCallback)(Characteristic*, bool)) {
  this->onSubscriptionChangedCallbacks.push_back(onSubscriptionChangedCallback);
}

