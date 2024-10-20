#ifndef DIRCON_H
#define DIRCON_H

#include <Arduino.h>
#include <NimBLEDevice.h>

#define DIRCON_MDNS_SERVICE_NAME "_wahoo-fitness-tnp"
#define DIRCON_MDNS_SERVICE_PROTOCOL "tcp"

std::vector<uint8_t> generateDirConPacket(uint8_t messageVersion, uint8_t identifier, uint8_t sequenceNumber, uint8_t responseCode, BLEUUID uuid, std::vector<uint8_t> data);

#endif