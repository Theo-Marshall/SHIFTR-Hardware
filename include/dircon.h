#ifndef DIRCON_H
#define DIRCON_H

#include <Arduino.h>
#include <NimBLEDevice.h>

#define DIRCON_MDNS_SERVICE_NAME "_wahoo-fitness-tnp"
#define DIRCON_MDNS_SERVICE_PROTOCOL "tcp"

uint8_t getDirConCharacteristicTypeFromBLEProperties(uint8_t bLEProperties); 
uint8_t getDirConCharacteristicTypeFromBLEProperties(NimBLERemoteCharacteristic *remoteCharacteristic); 

#endif