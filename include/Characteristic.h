#ifndef CHARACTERISTIC_H
#define CHARACTERISTIC_H

#include <NimBLEUUID.h>
#include <CharacteristicCallbacks.h>
#include <vector>
#include <Service.h>

class Service;

class Characteristic {
 public:
  friend class Service;
  Characteristic(NimBLEUUID uuid, uint32_t properties);
  NimBLEUUID UUID;
  uint32_t Properties;
  void addSubscription(uint32_t clientID);
  void removeSubscription(uint32_t clientID);
  bool isSubscribed(uint32_t clientID);
  std::vector<uint32_t> getSubscriptions();
  void subscribeCallbacks(CharacteristicCallbacks* callbacks);
  Service* getService();

 private:
  std::vector<uint32_t> subscriptions;
  std::vector<CharacteristicCallbacks*> characteristicCallbacks;
  Service* service;
  void doCallbacks(bool removed);

};

#endif
