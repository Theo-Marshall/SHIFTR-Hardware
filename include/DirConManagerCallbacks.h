#ifndef DIRCONMANAGERCALLBACKS_H
#define DIRCONMANAGERCALLBACKS_H

#include <Characteristic.h>
#include <TrainerMode.h>

class DirConManager;

class DirConManagerCallbacks {
 public:
  virtual ~DirConManagerCallbacks() {}
  virtual void onGearChanged(uint8_t currentGear, double currentGearRatio) {};
  virtual void onTrainerModeChanged(TrainerMode trainerMode) {};
};

#endif