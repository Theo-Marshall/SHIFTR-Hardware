#include <Arduino.h>
#include <DirConCharacteristic.h>

DirConCharacteristic::DirConCharacteristic(NimBLEUUID characteristicUUID, uint8_t characteristicType) 
{
  this->UUID = characteristicUUID;
  this->Type = characteristicType;
}
