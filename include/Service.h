#ifndef SERVICE_H
#define SERVICE_H

#include <vector>
#include <NimBLEUUID.h>
#include <Characteristic.h>

class Service {
  public:
    Service(NimBLEUUID uuid);
    Service(NimBLEUUID uuid, bool advertise, bool internal);
    Service(NimBLEUUID uuid, std::vector<Characteristic> characteristics, bool advertise, bool internal);
    void addCharacteristic(Characteristic characteristic);
    std::vector<Characteristic>* getCharacteristics();
    bool isAdvertised();
    Characteristic* getCharacteristic(NimBLEUUID characteristicUUID);
    NimBLEUUID UUID;
    std::vector<Characteristic> Characteristics;
    bool Advertise = false;
    bool Internal = false;
  private:
};

#endif
