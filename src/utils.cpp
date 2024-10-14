#include <Arduino.h>
#include <config.h>

const char *getMacAddressString()
{
  uint64_t macAddress = ESP.getEfuseMac();
  static char macAddressString[18];
  sprintf(macAddressString, "%02X-%02X-%02X-%02X-%02X-%02X", (uint8_t)(macAddress), (uint8_t)(macAddress >> 8), (uint8_t)(macAddress >> 16), (uint8_t)(macAddress >> 24), (uint8_t)(macAddress >> 32), (uint8_t)(macAddress >> 40));
  return macAddressString;
}

const char *getSerialNumberString()
{
  uint64_t macAddress = ESP.getEfuseMac();
  static char serialNumberString[16];
  sprintf(serialNumberString, "%i%i%i", (uint8_t)(macAddress >> 24), (uint8_t)(macAddress >> 32), (uint8_t)(macAddress >> 40));
  return serialNumberString;
}

char *getUniqueIdString()
{
  uint64_t macAddress = ESP.getEfuseMac();
  static char uniqueIdString[7];
  sprintf(uniqueIdString, "%02X%02X%02X", (uint8_t)(macAddress >> 24), (uint8_t)(macAddress >> 32), (uint8_t)(macAddress >> 40));
  return uniqueIdString;
}

char *getDeviceName()
{
  static char displayName[strlen(DEVICE_NAME_PREFIX) + 8];
  sprintf(displayName, "%s %s", DEVICE_NAME_PREFIX, getUniqueIdString());
  displayName[strlen(DEVICE_NAME_PREFIX) + 7] = 0x00;
  return displayName;
}

char *getHostName()
{
  char *deviceName = getDeviceName();
  static char hostName[strlen(DEVICE_NAME_PREFIX) + 8];
  for (uint8_t currentByte = 0; currentByte < (strlen(DEVICE_NAME_PREFIX) + 8); currentByte++) {
    if (deviceName[currentByte] != 0x20) {
      hostName[currentByte] = deviceName[currentByte];
    } else {
      hostName[currentByte] = 0x2D;
    }
  }
  return hostName;
}

String getHexString(uint8_t *data, size_t len) 
{
  char hexNumber[3];
  String hexString = "[";
  for (size_t counter = 0; counter < len; counter++)
  {
    sprintf(hexNumber, "%02X", data[counter]);
    hexString += hexNumber;
    if (counter < (len - 1))
    {
      hexString += " ";
    } 
  }
  hexString += "]";
  return hexString;
}

