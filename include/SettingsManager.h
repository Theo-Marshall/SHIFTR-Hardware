#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <Arduino.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h>

class SettingsManager {
 public:
  static void initialize();
  static bool isVirtualShiftingEnabled();
  static void setVirtualShiftingEnabled(bool enabled);
  static std::string getTrainerDeviceName();
  static void setTrainerDeviceName(std::string trainerDevice);
  static IotWebConfParameterGroup* getIoTWebConfSettingsParameterGroup();

 private:
  static char iotWebConfVirtualShiftingParameterValue[];
  static char iotWebConfTrainerDeviceParameterValue[];
  static IotWebConfParameterGroup iotWebConfSettingsGroup;
  static IotWebConfCheckboxParameter iotWebConfVirtualShiftingParameter;
  static IotWebConfTextParameter iotWebConfTrainerDeviceParameter;
};

#endif
