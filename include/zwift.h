#ifndef ZWIFT_H
#define ZWIFT_H

#include <Arduino.h>
#include <vector>

#define ZWIFT_MAGIC_WORD "RideOn"

std::vector<uint8_t> generateMagicWordAnswer();
std::vector<uint8_t> generateZwiftAsyncNotificationData(int64_t power, int64_t cadence, int64_t unknown1, int64_t unknown2, int64_t unknown3, int64_t unknown4 = 25714LL);

#endif