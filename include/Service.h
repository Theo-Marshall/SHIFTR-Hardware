#ifndef SERVICE_H
#define SERVICE_H

#include <vector>
#include <NimBLEUUID.h>
#include <Characteristic.h>

class Service {
  public:
    Service(NimBLEUUID uuid);
    Service(NimBLEUUID uuid, bool advertise);
    Service(NimBLEUUID uuid, std::vector<Characteristic> characteristics, bool advertise);
    NimBLEUUID UUID;
    std::vector<Characteristic> Characteristics;
    bool Advertise = false;
  private:
};

#endif
