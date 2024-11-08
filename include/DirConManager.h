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

typedef enum TrainerMode {
  ERG_MODE = 0,
  SIM_MODE = 1
} trainerModeType;

class DirConManager {
 public:
  static bool start();
  static void stop();
  static void update();
  static void setServiceManager(ServiceManager* serviceManager);
  static void notifyDirConCharacteristic(const NimBLEUUID& characteristicUUID, uint8_t* pData, size_t length);
  static int64_t getCurrentPower();
  static int64_t getCurrentCadence();
  static int64_t getCurrentInclination();
  static int64_t getCurrentGearRatio();
  static int64_t getCurrentRequestedPower();
  static int16_t getCurrentDevicePower();
  static uint16_t getCurrentDeviceCrankRevolutions();
  static uint16_t getCurrentDeviceCrankLastEventTime();
  static uint16_t getCurrentDeviceCadence();
  static uint16_t getCurrentDeviceGrade();
  static bool isVirtualShiftingEnabled();
  static void setCurrentPower(int64_t power);
  static void setCurrentCadence(int64_t cadence);
  static String getStatusMessage();

 private:
  friend class DirConServiceManagerCallbacks;
  static TrainerMode trainerMode;
  static uint8_t zwiftAsyncRideOnAnswer[];
  static uint8_t zwiftSyncRideOnAnswer[];
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
  static std::map<uint8_t, int64_t> getZwiftDataValues(std::vector<uint8_t>* requestData);
  static std::map<uint8_t, uint64_t> getUnsignedZwiftDataValues(std::vector<uint8_t>* requestData);
  static void sendDirConCharacteristicNotification(const NimBLEUUID& characteristicUUID, uint8_t* pData, size_t length, bool onlySubscribers);
  static void sendDirConCharacteristicNotification(Characteristic* characteristic, uint8_t* pData, size_t length, bool onlySubscribers);
  static void updateStatusMessage();
  static ServiceManager* serviceManager;
  static Timer<> notificationTimer;
  static AsyncServer* dirConServer;
  static AsyncClient* dirConClients[DIRCON_MAX_CLIENTS];
  static bool started;
  static int64_t currentPower;
  static int64_t currentCadence;
  static int64_t currentInclination;
  static int64_t currentGearRatio;
  static int64_t currentRequestedPower;
  static int16_t currentDevicePower;
  static uint16_t currentDeviceCrankRevolutions;
  static uint16_t currentDeviceCrankLastEventTime;
  static bool currentDeviceCrankStaleness;
  static uint16_t currentDeviceCadence;
  static uint8_t currentDeviceGearRatio;
  static uint8_t currentDeviceWheelDiameter;
  static uint16_t currentDeviceGrade;
  static bool virtualShiftingEnabled;
  static String statusMessage;
};

#endif
