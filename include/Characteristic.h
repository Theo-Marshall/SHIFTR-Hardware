#ifndef CHARACTERISTIC_H
#define CHARACTERISTIC_H

#include <NimBLEUUID.h>

#include <vector>

class Characteristic {
   public:
    Characteristic(NimBLEUUID uuid, uint32_t properties);
    NimBLEUUID UUID;
    uint32_t Properties;
   private:
};

#endif