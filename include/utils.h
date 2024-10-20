#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <NimBLEDevice.h>

const char *getMacAddressString();
const char *getSerialNumberString();
char *getUniqueIdString();
char *getDeviceName();
char *getHostName();
String getHexString(uint8_t *data, size_t len);
uint8_t getDirConCharacteristicTypeFromBLEProperties(uint8_t bLEProperties); 
uint8_t getDirConCharacteristicTypeFromBLEProperties(NimBLERemoteCharacteristic *remoteCharacteristic); 

#endif