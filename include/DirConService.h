#ifndef DIRCONSERVICE_H
#define DIRCONSERVICE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <DirConCharacteristic.h>

class DirConService {
  public:
    DirConService();
    DirConService(NimBLEUUID serviceUUID);
    NimBLEUUID UUID;
    std::vector<DirConCharacteristic> Characteristics;
    void addCharacteristic(NimBLEUUID characteristicUUID, uint8_t characteristicType);
  private:
};

#endif