#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

const char *getMacAddressString();
const char *getSerialNumberString();
char *getUniqueIdString();
char *getDeviceName();
char *getHostName();
String getHexString(uint8_t *data, size_t len); 

#endif