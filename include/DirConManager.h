#ifndef DIRCONMANAGER_H
#define DIRCONMANAGER_H

#define DIRCON_MDNS_SERVICE_NAME "_wahoo-fitness-tnp"
#define DIRCON_MDNS_SERVICE_PROTOCOL "tcp"

#include <AsyncTCP.h>
#include <Config.h>
#include <ServiceManager.h>
#include <arduino-timer.h>
#include <DirConMessage.h>
#include <UUIDs.h>

class DirConManager {
 public:
  static bool start();
  static void stop();
  static void update();
  static void setServiceManager(ServiceManager* serviceManager);
  static void notifyDirConCharacteristic(const NimBLEUUID& characteristicUUID, uint8_t* pData, size_t length);
  static int64_t getCurrentPower();
  static int64_t getCurrentCadence();
  static void setCurrentPower(int64_t power);
  static void setCurrentCadence(int64_t cadence);

 private:
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
  static std::vector<uint8_t> processZwiftSyncRequest(std::vector<uint8_t>* requestData);
  static void notifyDirConCharacteristic(Characteristic* characteristic, uint8_t* pData, size_t length);
  static void notifyInternalCharacteristics();
  static std::vector<uint8_t> generateZwiftAsyncNotificationData(int64_t power, int64_t cadence, int64_t unknown1, int64_t unknown2, int64_t unknown3, int64_t unknown4 = 25714LL);
  static ServiceManager* serviceManager;
  static Timer<> notificationTimer;
  static AsyncServer* dirConServer;
  static AsyncClient* dirConClients[DIRCON_MAX_CLIENTS];
  static bool started;
  static int64_t currentPower;
  static int64_t currentCadence;
};

#endif
