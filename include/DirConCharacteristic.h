#ifndef DIRCONCHARACTERISTIC_H
#define DIRCONCHARACTERISTIC_H

#include <Arduino.h>
#include <NimBLEDevice.h>

class DirConCharacteristic {
 public:
  DirConCharacteristic(NimBLEUUID characteristicUUID, uint8_t characteristicType);
  NimBLEUUID UUID;
  uint8_t Type;
  std::vector<uint8_t> NotificationSubscriptions;
  void subscribeNotification(uint8_t clientId);
  void subscribeNotification(uint8_t clientId, bool subscribe);
  void unSubscribeNotification(uint8_t clientId);

 private:
};

#endif