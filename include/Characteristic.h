#ifndef CHARACTERISTIC_H
#define CHARACTERISTIC_H

#include <NimBLEUUID.h>

#include <vector>

class Characteristic {
 public:
  Characteristic(NimBLEUUID uuid, uint32_t properties);
  NimBLEUUID UUID;
  uint32_t Properties;
  void addSubscription(uint32_t clientID);
  void removeSubscription(uint32_t clientID);
  bool isSubscribed(uint32_t clientID);
  std::vector<uint32_t> getSubscriptions();
  void subscribeOnSubscriptionChanged(void (*onSubscriptionChangeCallback)(Characteristic*, bool));

 private:
  std::vector<uint32_t> subscriptions;
  std::vector<void(*)(Characteristic* characteristic, bool removed)> onSubscriptionChangedCallbacks;

};

#endif
