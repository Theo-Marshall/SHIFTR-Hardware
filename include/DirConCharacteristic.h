#ifndef DIRCONCHARACTERISTIC_H
#define DIRCONCHARACTERISTIC_H

#include <Arduino.h>
#include <NimBLEDevice.h>

class DirConCharacteristic {
  public:
    DirConCharacteristic(NimBLEUUID characteristicUUID, uint8_t characteristicType);
    NimBLEUUID UUID;
    uint8_t Type;
  private:
};

#endif