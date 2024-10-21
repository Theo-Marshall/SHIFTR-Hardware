#include <Arduino.h>
#include <DirConCharacteristic.h>

DirConCharacteristic::DirConCharacteristic(NimBLEUUID characteristicUUID, uint8_t characteristicType) 
{
  this->UUID = characteristicUUID;
  this->Type = characteristicType;
}

void DirConCharacteristic::unSubscribeNotification(uint8_t clientId)
{
  std::vector<uint8_t> filteredSubscriptions;
  for (size_t index = 0; index < this->NotificationSubscriptions.size(); index++)
  {
    if (this->NotificationSubscriptions.at(index) != clientId) 
    {
      filteredSubscriptions.push_back(this->NotificationSubscriptions.at(index));
    }
  }
  this->NotificationSubscriptions.clear();
  for (size_t index = 0; index < filteredSubscriptions.size(); index++)
  {
    this->NotificationSubscriptions.push_back(filteredSubscriptions.at(index));
  }
}

void DirConCharacteristic::subscribeNotification(uint8_t clientId)
{
  bool clientidFound = false;
  for (size_t index = 0; index < this->NotificationSubscriptions.size(); index++)
  {
    if (this->NotificationSubscriptions.at(index) == clientId) 
    {
      clientidFound = true;
      break;
    }
  }
  if (!clientidFound) 
  {
    this->NotificationSubscriptions.push_back(clientId);
  }
}

void DirConCharacteristic::subscribeNotification(uint8_t clientId, bool subscribe)
{
  if (subscribe)
  {
    this->subscribeNotification(clientId);
  } else {
    this->unSubscribeNotification(clientId);
  }
}
