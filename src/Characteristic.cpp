#include <Characteristic.h>

Characteristic::Characteristic(NimBLEUUID uuid, uint32_t properties) 
{
  this->UUID = uuid;
  this->Properties = properties;
}