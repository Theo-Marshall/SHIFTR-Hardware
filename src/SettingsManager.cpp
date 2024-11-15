#include <SettingsManager.h>

char SettingsManager::iotWebConfChainringTeethParameterValue[16];
char SettingsManager::iotWebConfSprocketTeethParameterValue[16];
char SettingsManager::iotWebConfTrackResistanceParameterValue[16];
char SettingsManager::iotWebConfVirtualShiftingParameterValue[16];
char SettingsManager::iotWebConfTrainerDeviceParameterValue[128];
IotWebConfParameterGroup SettingsManager::iotWebConfSettingsGroup = IotWebConfParameterGroup("settings", "Device settings");

IotWebConfNumberParameter SettingsManager::iotWebConfChainringTeethParameter("Chainring teeth", "chainring_teeth", SettingsManager::iotWebConfChainringTeethParameterValue, sizeof(iotWebConfChainringTeethParameterValue), "34", "1..100", "min='1' max='100' step='1'");
IotWebConfNumberParameter SettingsManager::iotWebConfSprocketTeethParameter("Sprocket teeth", "sprocket_teeth", SettingsManager::iotWebConfSprocketTeethParameterValue, sizeof(iotWebConfSprocketTeethParameterValue), "14", "1..100", "min='1' max='100' step='1'");
IotWebConfCheckboxParameter SettingsManager::iotWebConfTrackResistanceParameter = IotWebConfCheckboxParameter("Use Track Resistance", "track_resistance", SettingsManager::iotWebConfTrackResistanceParameterValue, sizeof(iotWebConfTrackResistanceParameterValue), true);
IotWebConfCheckboxParameter SettingsManager::iotWebConfVirtualShiftingParameter = IotWebConfCheckboxParameter("Virtual shifting", "virtual_shifting", SettingsManager::iotWebConfVirtualShiftingParameterValue, sizeof(iotWebConfVirtualShiftingParameterValue), true);
IotWebConfTextParameter SettingsManager::iotWebConfTrainerDeviceParameter = IotWebConfTextParameter("Trainer device", "trainer_device", iotWebConfTrainerDeviceParameterValue, sizeof(iotWebConfTrainerDeviceParameterValue), "");

void SettingsManager::initialize() {
  iotWebConfSettingsGroup.addItem(&iotWebConfTrainerDeviceParameter);
  iotWebConfSettingsGroup.addItem(&iotWebConfVirtualShiftingParameter);
  iotWebConfSettingsGroup.addItem(&iotWebConfTrackResistanceParameter);
  iotWebConfSettingsGroup.addItem(&iotWebConfChainringTeethParameter);
  iotWebConfSettingsGroup.addItem(&iotWebConfSprocketTeethParameter);
}

uint16_t SettingsManager::getChainringTeeth() {
  return atoi(iotWebConfChainringTeethParameterValue);
}

void SettingsManager::setChainringTeeth(uint16_t chainringTeeth) {
  std::string chainringTeethString = std::to_string(chainringTeeth);
  strncpy(iotWebConfChainringTeethParameterValue, chainringTeethString.c_str(), sizeof(iotWebConfChainringTeethParameterValue));
}

uint16_t SettingsManager::getSprocketTeeth() {
  return atoi(iotWebConfSprocketTeethParameterValue);
}

void SettingsManager::setSprocketTeeth(uint16_t sprocketTeeth) {
  std::string sprocketTeethString = std::to_string(sprocketTeeth);
  strncpy(iotWebConfSprocketTeethParameterValue, sprocketTeethString.c_str(), sizeof(iotWebConfSprocketTeethParameterValue));
}

bool SettingsManager::isTrackResistanceEnabled() {
  return (strncmp(iotWebConfTrackResistanceParameterValue, "selected", sizeof(iotWebConfTrackResistanceParameterValue)) == 0);
}

bool SettingsManager::isVirtualShiftingEnabled() {
  return (strncmp(iotWebConfVirtualShiftingParameterValue, "selected", sizeof(iotWebConfVirtualShiftingParameterValue)) == 0);
}

void SettingsManager::setTrackResistanceEnabled(bool enabled) {
  String trackResistanceEnabled = "";
  if (enabled) {
    trackResistanceEnabled = "selected";
  }
  strncpy(iotWebConfTrackResistanceParameterValue, trackResistanceEnabled.c_str(), sizeof(iotWebConfTrackResistanceParameterValue));
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