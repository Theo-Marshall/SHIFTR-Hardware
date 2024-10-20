#include <Arduino.h>
#include <NimBLEDevice.h>
#include <DirCon.h>
#include <DirConMessage.h>

uint8_t getDirConCharacteristicTypeFromBLEProperties(uint16_t bLEProperties)
{
  uint8_t returnValue = 0x00;
  if ((bLEProperties && NIMBLE_PROPERTY::READ) == NIMBLE_PROPERTY::READ) 
  {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_READ;
  }
  if (((bLEProperties && NIMBLE_PROPERTY::WRITE) == NIMBLE_PROPERTY::WRITE) || ((bLEProperties && NIMBLE_PROPERTY::WRITE_NR) == NIMBLE_PROPERTY::WRITE_NR))
  {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_WRITE;
  }
  if (((bLEProperties && NIMBLE_PROPERTY::NOTIFY) == NIMBLE_PROPERTY::NOTIFY) || ((bLEProperties && NIMBLE_PROPERTY::INDICATE) == NIMBLE_PROPERTY::INDICATE))
  {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_NOTIFY;
  }

  return returnValue;
}

uint8_t getDirConCharacteristicTypeFromBLEProperties(NimBLERemoteCharacteristic *remoteCharacteristic) 
{
  uint8_t returnValue = 0x00;
  if (remoteCharacteristic->canRead()) 
  {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_READ;
  }
  if (remoteCharacteristic->canWrite() || remoteCharacteristic->canWriteNoResponse())
  {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_WRITE;
  }
  if (remoteCharacteristic->canIndicate() || remoteCharacteristic->canNotify() || remoteCharacteristic->canBroadcast())
  {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_NOTIFY;
  }

  return returnValue;
}
