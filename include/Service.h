#ifndef SERVICE_H
#define SERVICE_H

#include <vector>
#include <NimBLEUUID.h>
#include <Characteristic.h>

class Service {
  public:
    Service(NimBLEUUID uuid);
    Service(NimBLEUUID uuid, bool advertise, bool internal);
    void addCharacteristic(Characteristic characteristic);
    std::vector<Characteristic>* getCharacteristics();
    bool isAdvertised();
    Characteristic* getCharacteristic(NimBLEUUID characteristicUUID);
    void subscribeOnSubscriptionChanged(void (*onSubscriptionChangeCallback)(Service*, Characteristic*, bool));
    NimBLEUUID UUID;
    std::vector<Characteristic> Characteristics;
    bool Advertise = false;
    bool Internal = false;
  private:
    void handleCharacteristicOnSubscriptionChanged(Characteristic* characteristic, bool removed);
    std::vector<void(*)(Service* service, Characteristic* characteristic, bool removed)> onSubscriptionChangedCallbacks;


};

#endif
