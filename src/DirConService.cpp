#include <Arduino.h>
#include <DirConService.h>

DirConService::DirConService() {}

DirConService::DirConService(NimBLEUUID serviceUUID) 
{
  this->UUID=serviceUUID;
}

void DirConService::addCharacteristic(NimBLEUUID characteristicUUID, uint8_t characteristicType)
{
  bool characteristicFound = false;
  for (size_t index = 0; index < this->Characteristics.size(); index++)
  {
    if (this->Characteristics.at(index).UUID.equals(characteristicUUID))
    {
      characteristicFound = true;
      this->Characteristics.at(index).Type = characteristicType;
      break;
    }
  }
  if (!characteristicFound) 
  {
    this->Characteristics.push_back(DirConCharacteristic(characteristicUUID, characteristicType));
  }  
}
