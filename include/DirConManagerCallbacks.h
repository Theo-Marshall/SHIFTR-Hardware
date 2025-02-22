#ifndef DIRCONMANAGERCALLBACKS_H
#define DIRCONMANAGERCALLBACKS_H

#include <Characteristic.h>

class DirConManager;

class DirConManagerCallbacks {
 public:
  virtual ~DirConManagerCallbacks() {}
  virtual void onGearChanged(uint8_t currentGear) {};
};

#endif