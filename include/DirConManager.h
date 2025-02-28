#ifndef DIRCONMANAGER_H
#define DIRCONMANAGER_H

#define DIRCON_MDNS_SERVICE_NAME "_wahoo-fitness-tnp"
#define DIRCON_MDNS_SERVICE_PROTOCOL "tcp"

#include <AsyncTCP.h>
#include <Config.h>
#include <DirConMessage.h>
#include <ServiceManager.h>
#include <UUIDs.h>
#include <arduino-timer.h>
#include <SettingsManager.h>
#include <deque>
#include "TrainerMode.h"
#include "DirConManagerCallbacks.h"

class DirConManager {
 public:
  static bool start();
  static void stop();
  static void update();
  static void setServiceManager(ServiceManager* serviceManager);
  static void notifyDirConCharacteristic(const NimBLEUUID& characteristicUUID, uint8_t* pData, size_t length);
  static String getStatusMessage();
  static TrainerMode getZwiftTrainerMode();
  static void shiftGearUp();
  static void shiftGearDown();
  static uint8_t getCurrentGear();
  static double getCurrentGearRatio();
  static void subscribeCallbacks(DirConManagerCallbacks* callbacks);

 private:
  friend class DirConServiceManagerCallbacks;
  static bool doNotifications(void* arg);
  static void handleNewClient(void* arg, AsyncClient* client);
  static void handleDirConData(void* arg, AsyncClient* client, void* data, size_t len);
  static void handleDirConError(void* arg, AsyncClient* client, int8_t error);
  static void handleDirConDisconnect(void* arg, AsyncClient* client);
  static void handleDirConTimeOut(void* arg, AsyncClient* client, uint32_t time);
  static void removeSubscriptions(AsyncClient* client);
  static size_t getDirConClientIndex(AsyncClient* client);
  static bool processDirConMessage(DirConMessage* dirConMessage, AsyncClient* client, size_t clientIndex);
  static uint8_t getDirConProperties(uint32_t characteristicProperties);
  static bool processZwiftSyncRequest(Service* service, Characteristic* characteristic, std::vector<uint8_t>* requestData);
  static void notifyDirConCharacteristic(Characteristic* characteristic, uint8_t* pData, size_t length);
  static void notifyInternalCharacteristics();
  static std::vector<uint8_t> generateZwiftAsyncNotificationData(int64_t power, int64_t cadence, int64_t unknown1, int64_t unknown2, int64_t unknown3, int64_t unknown4 = 25714LL);
  static std::map<uint8_t, uint64_t> getZwiftDataValues(std::vector<uint8_t>* requestData);
  static void sendDirConCharacteristicNotification(const NimBLEUUID& characteristicUUID, uint8_t* pData, size_t length, bool onlySubscribers);
  static void sendDirConCharacteristicNotification(Characteristic* characteristic, uint8_t* pData, size_t length, bool onlySubscribers);
  static void updateStatusMessage();
  static void resetValues();
  static void updateZwiftSIMModeResistance();
  static void updateZwiftlessSIMModeResistance();
  static std::vector<uint8_t> processFTMSReadRequest(Service* service, Characteristic* characteristic, std::vector<uint8_t>* requestData); 
  static std::vector<uint8_t> processFTMSWriteRequest(Service* service, Characteristic* characteristic, std::vector<uint8_t>* requestData);
  static std::vector<uint8_t> generateIndoorBikeDataNotificationData(uint16_t instantaneousSpeed, uint16_t instantaneousCadence, int16_t instantaneousPower);
  static void doGearChangeCallbacks(uint8_t currentGear, double currentGearRatio);
  static void setGearByNumber(uint8_t gearNumber);
  static void setGearByRatio(double gearRatio);
  static void doTrainerModeChangeCallbacks(TrainerMode trainerMode);
  static void updateSIMModeParameters(TrainerMode trainerMode, bool zwiftlessShifting, double bicycleWeight, double userWeight, double grade, uint8_t crr, double measuredSpeed, uint8_t cadence, double gearRatio, double defaultGearRatio, uint16_t difficulty, uint16_t maximumResistance);
  static void initializeSIMModeUserConfiguration();

  static std::vector<DirConManagerCallbacks*> dirConManagerCallbacks;
  
  static ServiceManager* serviceManager;
  static Timer<> notificationTimer;
  static AsyncServer* dirConServer;
  static AsyncClient* dirConClients[DIRCON_MAX_CLIENTS];
  static bool started;
  static String statusMessage;
  static uint8_t zwiftAsyncRideOnAnswer[];
  static uint8_t zwiftSyncRideOnAnswer[];

  static double defaultGearRatio;
  static VirtualShiftingMode virtualShiftingMode;

  static TrainerMode zwiftTrainerMode;
  static uint64_t zwiftPower;
  static int64_t zwiftGrade;
  static uint64_t zwiftGearRatio;
  static uint16_t zwiftBicycleWeight;
  static uint16_t zwiftUserWeight;

  static int64_t smoothedZwiftGrade;

  static uint16_t trainerInstantaneousPower;
  static uint16_t trainerInstantaneousSpeed;
  static uint8_t trainerCadence;
  static uint16_t trainerMaximumResistance;

  static uint16_t difficulty;

  static int16_t ftmsPower;
  static int16_t ftmsWindSpeed;
  static int16_t ftmsGrade;
  static uint8_t ftmsCrr;
  static uint8_t ftmsCw;

  static double manualGears[];
  static uint8_t currentGear;
  static double currentGearRatio;
  static bool zwiftlessShiftingEnabled;

};

#endif
