#include <Service.h>

Service::Service(NimBLEUUID uuid)
{
  this->UUID = uuid;
}

Service::Service(NimBLEUUID uuid, bool advertise)
{
  this->UUID = uuid;
  this->Advertise = advertise;
}

Service::Service(NimBLEUUID uuid, std::vector<Characteristic> characteristics, bool advertise)
{
  this->UUID = uuid;
  this->Characteristics = characteristics;
  this->Advertise = advertise;
}
