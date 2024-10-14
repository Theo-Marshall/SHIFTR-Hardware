#include <Arduino.h>
#include <NimBLEDevice.h>
#include <dircon.h>

std::vector<uint8_t> generateDirConPacket(uint8_t messageVersion, uint8_t identifier, uint8_t sequenceNumber, uint8_t responseCode, BLEUUID uuid, std::vector<uint8_t> data) {
  std::vector<uint8_t> dirConPacket;
  uint16_t length = 16;
  uint8_t *uuidBytes = (uint8_t*)uuid.getNative();

  dirConPacket.push_back(messageVersion);
  dirConPacket.push_back(identifier);
  dirConPacket.push_back(sequenceNumber);
  dirConPacket.push_back(responseCode);
  length += data.size();
  dirConPacket.push_back((uint8_t)(length >> 8));
  dirConPacket.push_back((uint8_t)(length));

  for (uint8_t i = 16; i > 0; i--)
  {
    dirConPacket.push_back(uuidBytes[i]);
  }
  for (uint8_t i = 0; i < data.size(); i++)
  {
    dirConPacket.push_back(data[i]);
  }

  return dirConPacket;
}