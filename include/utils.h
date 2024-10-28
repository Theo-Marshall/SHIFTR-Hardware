#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <NimBLEDevice.h>

class Utils {
 public:
  static std::string getHexString(uint8_t* data, size_t length);
  static std::string getHexString(std::vector<uint8_t> data);
  static std::string getMacAddressString();
  static std::string getSerialNumberString();
  static std::string getDeviceName();
  static std::string getHostName();

 private:
  static uint64_t macAddress;
};

#endif