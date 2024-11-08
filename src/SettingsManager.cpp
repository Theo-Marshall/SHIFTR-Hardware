#include <SettingsManager.h>

char SettingsManager::iotWebConfVirtualShiftingParameterValue[16];
char SettingsManager::iotWebConfTrainerDeviceParameterValue[128];
IotWebConfParameterGroup SettingsManager::iotWebConfSettingsGroup = IotWebConfParameterGroup("settings", "Device settings");
IotWebConfCheckboxParameter SettingsManager::iotWebConfVirtualShiftingParameter = IotWebConfCheckboxParameter("Virtual shifting", "virtual_shifting", SettingsManager::iotWebConfVirtualShiftingParameterValue, sizeof(iotWebConfVirtualShiftingParameterValue), true);
IotWebConfTextParameter SettingsManager::iotWebConfTrainerDeviceParameter = IotWebConfTextParameter("Trainer device", "trainer_device", iotWebConfTrainerDeviceParameterValue, sizeof(iotWebConfTrainerDeviceParameterValue), "");

void SettingsManager::initialize() {
  iotWebConfSettingsGroup.addItem(&iotWebConfTrainerDeviceParameter);
  iotWebConfSettingsGroup.addItem(&iotWebConfVirtualShiftingParameter);
}

bool SettingsManager::isVirtualShiftingEnabled() {
  return (strncmp(iotWebConfVirtualShiftingParameterValue, "selected", sizeof(iotWebConfVirtualShiftingParameterValue)) == 0);
}

void SettingsManager::setVirtualShiftingEnabled(bool enabled) {
  String virtualShiftingEnabled = "";
  if (enabled) {
    virtualShiftingEnabled = "selected";
  }
  strncpy(iotWebConfVirtualShiftingParameterValue, virtualShiftingEnabled.c_str(), sizeof(iotWebConfVirtualShiftingParameterValue));
}

std::string SettingsManager::getTrainerDeviceName() {
  return std::string(iotWebConfTrainerDeviceParameterValue);
}

void SettingsManager::setTrainerDeviceName(std::string trainerDevice) {
  strncpy(iotWebConfTrainerDeviceParameterValue, trainerDevice.c_str(), sizeof(iotWebConfTrainerDeviceParameterValue));
}

IotWebConfParameterGroup* SettingsManager::getIoTWebConfSettingsParameterGroup() {
  return &iotWebConfSettingsGroup;
}