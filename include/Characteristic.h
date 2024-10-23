#ifndef CHARACTERISTIC_H
#define CHARACTERISTIC_H

#include <vector>
#include <NimBLEUUID.h>

class Characteristic {
  public:
    Characteristic(NimBLEUUID uuid, uint32_t properties);
    NimBLEUUID UUID;
    uint32_t Properties;
  private:
};

#endif