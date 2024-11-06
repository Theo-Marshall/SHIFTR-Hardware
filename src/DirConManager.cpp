#include <Arduino.h>
#include <BTDeviceManager.h>
#include <Config.h>
#include <DirConManager.h>
#include <ESPmDNS.h>
#include <Service.h>
#include <ServiceManagerCallbacks.h>
#include <Utils.h>
#include <leb128.h>

uint8_t DirConManager::zwiftAsyncRideOnAnswer[18] = {0x2a, 0x08, 0x03, 0x12, 0x0d, 0x22, 0x0b, 0x52, 0x49, 0x44, 0x45, 0x5f, 0x4f, 0x4e, 0x28, 0x30, 0x29, 0x00};
uint8_t DirConManager::zwiftSyncRideOnAnswer[8] = {0x52, 0x69, 0x64, 0x65, 0x4f, 0x6e, 0x02, 0x00};

ServiceManager *DirConManager::serviceManager{};
Timer<> DirConManager::notificationTimer = timer_create_default();
bool DirConManager::started = false;
AsyncServer *DirConManager::dirConServer = new AsyncServer(DIRCON_TCP_PORT);
AsyncClient *DirConManager::dirConClients[];
int64_t DirConManager::currentPower = 0;
int64_t DirConManager::currentCadence = 0;
int64_t DirConManager::currentInclination = 0;
int64_t DirConManager::currentGearRatio = 0;
int64_t DirConManager::currentRequestedPower = 0;
int16_t DirConManager::currentDevicePower = 0;
uint16_t DirConManager::currentDeviceCrankRevolutions = 0;
uint16_t DirConManager::currentDeviceCrankLastEventTime = 0;
bool DirConManager::currentDeviceCrankStaleness = true;
uint16_t DirConManager::currentDeviceCadence = 0;
uint8_t DirConManager::currentDeviceGearRatio = 0;
uint8_t DirConManager::currentDeviceWheelDiameter = 0;
uint16_t DirConManager::currentDeviceGrade = 0;
bool DirConManager::virtualShiftingEnabled = false;
String DirConManager::debugMessage = "";

class DirConServiceManagerCallbacks : public ServiceManagerCallbacks {
  void onServiceAdded(Service *service) {
    if (service->isAdvertised()) {
      String serviceUUIDs = "";
      for (Service *currentService : service->getServiceManager()->getServices()) {
        if (currentService->isAdvertised()) {
          if (serviceUUIDs != "") {
            serviceUUIDs += ",";
          }
          serviceUUIDs += currentService->UUID.to16().toString().c_str();
        }
      }
      log_i("Updating advertised service UUIDs: %s", serviceUUIDs.c_str());
      MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "ble-service-uuids", serviceUUIDs.c_str());
    };
  };

  void onCharacteristicSubscriptionChanged(Characteristic *characteristic, bool removed) {
    if (characteristic->UUID.equals(NimBLEUUID(CYCLING_POWER_MEASUREMENT_CHARACTERISTIC_UUID))) {
      if (removed && (characteristic->getSubscriptions().size() == 0)) {
        DirConManager::currentDeviceCrankStaleness = true;
        DirConManager::currentDeviceCrankRevolutions = 0;
        DirConManager::currentDeviceCrankLastEventTime = 0;
        DirConManager::currentDeviceCadence = 0;
        DirConManager::currentDevicePower = 0;
        DirConManager::currentDeviceGearRatio = 0;
        DirConManager::currentDeviceWheelDiameter = 0;
      } else {
        if (characteristic->getSubscriptions().size() > 0) {
          DirConManager::currentDeviceCrankStaleness = true;
        }
      }
    }
  };
};

void DirConManager::setServiceManager(ServiceManager *serviceManager) {
  DirConManager::serviceManager = serviceManager;
}

bool DirConManager::start() {
  if (!started) {
    if (serviceManager == nullptr) {
      return false;
    }
    notificationTimer.every(DIRCON_NOTIFICATION_INTERVAL, DirConManager::doNotifications);
    serviceManager->subscribeCallbacks(new DirConServiceManagerCallbacks);
    MDNS.addService(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, DIRCON_TCP_PORT);
    MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "mac-address", Utils::getMacAddressString().c_str());
    MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "serial-number", Utils::getSerialNumberString().c_str());
    MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "ble-service-uuids", "");
    dirConServer->begin();
    dirConServer->onClient(&handleNewClient, dirConServer);
    started = true;
    return true;
  }
  return false;
}

void DirConManager::stop() {
  dirConServer->end();
  notificationTimer.cancel();
  MDNS.end();
  MDNS.begin(Utils::getHostName().c_str());
  MDNS.setInstanceName(Utils::getDeviceName().c_str());
}

void DirConManager::update() {
  notificationTimer.tick();
}

bool DirConManager::doNotifications(void *arg) {
  if (started) {
    notifyInternalCharacteristics();
  }
  return true;
}

void DirConManager::handleNewClient(void *arg, AsyncClient *client) {
  bool clientAccepted = false;
  log_i("New DirCon connection from %s", client->remoteIP().toString().c_str());
  for (size_t clientIndex = 0; clientIndex < DIRCON_MAX_CLIENTS; clientIndex++) {
    if ((dirConClients[clientIndex] == nullptr) || ((dirConClients[clientIndex] != nullptr) && (!dirConClients[clientIndex]->connected()))) {
      dirConClients[clientIndex] = client;
      clientAccepted = true;
      break;
    }
  }
  if (clientAccepted) {
    log_i("Free connection slot found, DirCon connection from %s accepted", client->remoteIP().toString().c_str());
    client->onData(&DirConManager::handleDirConData, NULL);
    client->onError(&DirConManager::handleDirConError, NULL);
    client->onDisconnect(&DirConManager::handleDirConDisconnect, NULL);
    client->onTimeout(&DirConManager::handleDirConTimeOut, NULL);
  } else {
    log_e("Maximum number of %d connections reached, DirCon connection from %s rejected", DIRCON_MAX_CLIENTS, client->remoteIP().toString().c_str());
    client->abort();
  }
}

void DirConManager::handleDirConData(void *arg, AsyncClient *client, void *data, size_t len) {
  size_t clientIndex = DirConManager::getDirConClientIndex(client);
  if (clientIndex != (DIRCON_MAX_CLIENTS + 1)) {
    log_d("DirCon client #%d with IP %s sent data, length: %d, hex value: %s", clientIndex, client->remoteIP().toString().c_str(), len, Utils::getHexString((uint8_t *)data, len).c_str());
    DirConMessage currentMessage;
    size_t parsedBytes = 0;
    while (parsedBytes < len) {
      log_d("Parsing from %d with length %d", parsedBytes, (len - parsedBytes));
      parsedBytes += currentMessage.parse((uint8_t *)data + parsedBytes, (len - parsedBytes), 0);
      if (currentMessage.Identifier == DIRCON_MSGID_ERROR) {
        log_e("Error handling data from DirCon client: Unable to parse DirCon message");
        parsedBytes += 1;
        if ((len - parsedBytes) <= 0) {
          return;
        }
      }
      if (!DirConManager::processDirConMessage(&currentMessage, client, clientIndex)) {
        log_e("Error handling data from DirCon client: Unable to process DirCon message");
      }
    }
  } else {
    log_e("Error handling data from DirCon client: Client slot not found");
  }
}

void DirConManager::handleDirConError(void *arg, AsyncClient *client, int8_t error) {
  log_e("DirCon client connection error %s from %s, stopping client...", client->errorToString(error), client->remoteIP().toString().c_str());
  removeSubscriptions(client);
  client->stop();
}

void DirConManager::handleDirConDisconnect(void *arg, AsyncClient *client) {
  log_i("DirCon client disconnected");
  removeSubscriptions(client);
}

void DirConManager::handleDirConTimeOut(void *arg, AsyncClient *client, uint32_t time) {
  log_e("DirCon client ACK timeout from %s, stopping client...", client->remoteIP().toString().c_str());
  removeSubscriptions(client);
  client->stop();
}

void DirConManager::removeSubscriptions(AsyncClient *client) {
  size_t clientIndex = DirConManager::getDirConClientIndex(client);
  if (clientIndex != (DIRCON_MAX_CLIENTS + 1)) {
    for (Service *service : serviceManager->getServices()) {
      for (Characteristic *characteristic : service->getCharacteristics()) {
        characteristic->removeSubscription(clientIndex);
      }
    }
  }
}

size_t DirConManager::getDirConClientIndex(AsyncClient *client) {
  for (size_t clientIndex = 0; clientIndex < DIRCON_MAX_CLIENTS; clientIndex++) {
    if (dirConClients[clientIndex] != nullptr) {
      if (dirConClients[clientIndex] == client) {
        return clientIndex;
        break;
      }
    }
  }
  return DIRCON_MAX_CLIENTS + 1;
}

bool DirConManager::processDirConMessage(DirConMessage *dirConMessage, AsyncClient *client, size_t clientIndex) {
  Service *service = nullptr;
  Characteristic *characteristic = nullptr;
  std::vector<uint8_t> *returnData;
  DirConMessage returnMessage;
  returnMessage.Identifier = dirConMessage->Identifier;
  returnMessage.UUID = dirConMessage->UUID;
  switch (dirConMessage->Identifier) {
    case DIRCON_MSGID_DISCOVER_SERVICES:
      log_d("DirCon service discovery request, returning services");
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      for (Service *currentService : serviceManager->getServices()) {
        returnMessage.AdditionalUUIDs.push_back(currentService->UUID);
      }
      break;
    case DIRCON_MSGID_DISCOVER_CHARACTERISTICS:
      log_d("DirCon characteristic discovery request for service UUID %s, returning characteristics", dirConMessage->UUID.to128().toString().c_str());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      service = serviceManager->getService(dirConMessage->UUID);
      if (service == nullptr) {
        returnMessage.ResponseCode = DIRCON_RESPCODE_SERVICE_NOT_FOUND;
        break;
      }
      for (Characteristic *characteristic : service->getCharacteristics()) {
        returnMessage.AdditionalUUIDs.push_back(characteristic->UUID);
        returnMessage.AdditionalData.push_back(getDirConProperties(characteristic->getProperties()));
      }
      break;
    case DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS:
      log_d("DirCon characteristic enable notification request for characteristic UUID %s", dirConMessage->UUID.to128().toString().c_str());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      characteristic = serviceManager->getCharacteristic(dirConMessage->UUID);
      if (characteristic == nullptr) {
        returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_NOT_FOUND;
        break;
      }
      if (dirConMessage->AdditionalData.size() != 1) {
        returnMessage.Identifier = DIRCON_RESPCODE_CHARACTERISTIC_OPERATION_NOT_SUPPORTED;
        break;
      }
      if (dirConMessage->AdditionalData.at(0) == 0) {
        if (characteristic->isSubscribed(clientIndex)) {
          characteristic->removeSubscription(clientIndex);
        }
      } else {
        if (!characteristic->isSubscribed(clientIndex)) {
          characteristic->addSubscription(clientIndex);
        }
      }
      break;
    case DIRCON_MSGID_WRITE_CHARACTERISTIC:
      log_d("DirCon write characteristic request for characteristic UUID %s", dirConMessage->UUID.to128().toString().c_str());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      characteristic = serviceManager->getCharacteristic(dirConMessage->UUID);
      if (characteristic == nullptr) {
        returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_NOT_FOUND;
        break;
      }
      if (dirConMessage->AdditionalData.size() == 0) {
        returnMessage.Identifier = DIRCON_RESPCODE_CHARACTERISTIC_OPERATION_NOT_SUPPORTED;
        break;
      }
      if (characteristic->getService() != nullptr) {
        if (!characteristic->getService()->isInternal()) {
          log_i("BLE write characteristic %s with data %s", characteristic->UUID.toString().c_str(), Utils::getHexString(dirConMessage->AdditionalData).c_str());
          if (!BTDeviceManager::writeBLECharacteristic(characteristic->getService()->UUID, characteristic->UUID, &(dirConMessage->AdditionalData))) {
            returnMessage.Identifier = DIRCON_RESPCODE_CHARACTERISTIC_OPERATION_NOT_SUPPORTED;
            break;
          }
        } else {
          if (!processZwiftSyncRequest(characteristic->getService(), characteristic, &dirConMessage->AdditionalData)) {
            returnMessage.Identifier = DIRCON_RESPCODE_CHARACTERISTIC_OPERATION_NOT_SUPPORTED;
            break;
          }
          break;
        }
      } else {
        returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_NOT_FOUND;
        break;
      }
      break;
    case DIRCON_MSGID_READ_CHARACTERISTIC:
      log_d("DirCon read characteristic request for characteristic UUID %s", dirConMessage->UUID.to128().toString().c_str());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      characteristic = serviceManager->getCharacteristic(dirConMessage->UUID);
      if (characteristic == nullptr) {
        returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_NOT_FOUND;
        break;
      }
      if (characteristic->getService() != nullptr) {
        if (!characteristic->getService()->isInternal()) {
          returnMessage.AdditionalData = BTDeviceManager::readBLECharacteristic(characteristic->getService()->UUID, characteristic->UUID);
          if (returnMessage.AdditionalData.size() == 0) {
            returnMessage.Identifier = DIRCON_RESPCODE_CHARACTERISTIC_OPERATION_NOT_SUPPORTED;
            break;
          }
        } else {
          log_e("DirCon read to internal characteristic, not implemented!");
          // TODO
        }
      } else {
        returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_NOT_FOUND;
        break;
      }

      break;
    default:
      log_e("Unknown identifier %d in DirCon message, skipping further processing", dirConMessage->Identifier);
      return false;
      break;
  }
  returnData = returnMessage.encode(dirConMessage->SequenceNumber);
  if (returnData->size() > 0) {
    // log_i("Sending data to DirCon client #%d with IP %s, length: %d, hex value: %s", clientIndex, client->remoteIP().toString().c_str(), returnData->size(), Utils::getHexString(returnData->data(), returnData->size()).c_str());
    client->write((char *)returnData->data(), returnData->size());
  } else {
    log_e("Error encoding DirCon message, aborting");
    return false;
  }

  return true;
}

uint8_t DirConManager::getDirConProperties(uint32_t characteristicProperties) {
  uint8_t returnValue = 0x00;
  if ((characteristicProperties & READ) == READ) {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_READ;
  }
  if ((characteristicProperties & WRITE) == WRITE) {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_WRITE;
  }
  if ((characteristicProperties & NOTIFY) == NOTIFY) {
    returnValue = returnValue | DIRCON_CHAR_PROP_FLAG_NOTIFY;
  }
  return returnValue;
}

std::map<uint8_t, int64_t> DirConManager::getZwiftDataValues(std::vector<uint8_t> *requestData) {
  std::map<uint8_t, int64_t> returnMap;
  if (requestData->size() > 2) {
    if (requestData->at(0) == 0x04) {
      size_t processedBytes = 0;
      size_t dataIndex = 0;
      uint8_t currentKey = 0;
      int64_t currentValue = 0;
      if (((requestData->at(1) == 0x22) || (requestData->at(1) == 0x2A)) && requestData->size() > 4) {
        dataIndex = 3;
        if (requestData->size() == (requestData->at(2) + dataIndex)) {
          while (dataIndex < requestData->size()) {
            currentKey = requestData->at(dataIndex);
            dataIndex++;
            processedBytes = bfs::DecodeLeb128(requestData->data() + dataIndex, requestData->size() - dataIndex, &currentValue);
            dataIndex = dataIndex + processedBytes;
            if (processedBytes == 0) {
              log_e("Error parsing Zwift data values, hex: ", Utils::getHexString(requestData).c_str());
              dataIndex++;
            }
            returnMap.emplace(currentKey, currentValue);
          }
        } else {
          log_e("Error parsing Zwift data values, length mismatch");
        }
      } else if (requestData->at(1) == 0x18) {
        dataIndex = 2;
        processedBytes = bfs::DecodeLeb128(requestData->data() + dataIndex, requestData->size() - dataIndex, &currentValue);
        if (processedBytes == 0) {
          log_e("Error parsing Zwift data value, hex: ", Utils::getHexString(requestData).c_str());
        } else {
          returnMap.emplace(currentKey, currentValue);
        }
      }
    }
  }
  return returnMap;
}

bool DirConManager::processZwiftSyncRequest(Service *service, Characteristic *characteristic, std::vector<uint8_t> *requestData) {
  uint8_t checksum = 0;
  std::vector<uint8_t> returnData;
  if (requestData->size() >= 3) {
    uint8_t zwiftCommand = requestData->at(0);
    uint8_t zwiftCommandSubtype = requestData->at(1);
    uint8_t zwiftCommandLength = requestData->at(2);
    std::map<uint8_t, int64_t> requestValues = getZwiftDataValues(requestData);

    log_i("Zwift command: %d, commandsubtype: %d", zwiftCommand, zwiftCommandSubtype);
    for (auto requestValue = requestValues.begin(); requestValue != requestValues.end(); requestValue++) {
      log_i("Zwift sync data key: %d, value %d", requestValue->first, requestValue->second);
    }

    switch (zwiftCommand) {
      // Status request
      case 0x00:
        return true;
        break;

        /*


        [285003][I][DirConManager.cpp:371] processZwiftSyncRequest(): Zwift command: 4, commandsubtype: 24
        [285012][I][DirConManager.cpp:387] processZwiftSyncRequest(): Hex: [04 18 73]
        [286111][I][DirConManager.cpp:371] processZwiftSyncRequest(): Zwift command: 4, commandsubtype: 24
        [286120][I][DirConManager.cpp:387] processZwiftSyncRequest(): Hex: [04 18 73]
        [286212][I][DirConManager.cpp:371] processZwiftSyncRequest(): Zwift command: 4, commandsubtype: 42
        [286221][I][DirConManager.cpp:373] processZwiftSyncRequest(): Zwift sync data key: 16, value 16800
        [286232][I][DirConManager.cpp:373] processZwiftSyncRequest(): Zwift sync data key: 32, value 616
        [286243][I][DirConManager.cpp:373] processZwiftSyncRequest(): Zwift sync data key: 40, value -5598
        [286254][I][DirConManager.cpp:456] processZwiftSyncRequest(): TACX Hex: [A4 09 4F 05 33 FF FF FF FF A0 41 53 66]
        [287018][I][DirConManager.cpp:371] processZwiftSyncRequest(): Zwift command: 4, commandsubtype: 34
        [287027][I][DirConManager.cpp:373] processZwiftSyncRequest(): Zwift sync data key: 16, value 200
        [287825][I][DirConManager.cpp:371] processZwiftSyncRequest(): Zwift command: 4, commandsubtype: 34
        [287834][I][DirConManager.cpp:373] processZwiftSyncRequest(): Zwift sync data key: 16, value 200

        */

      // Change request
      case 0x04:
        switch (zwiftCommandSubtype) {
          // ERG Mode
          case 0x18:
            log_i("ERG mode enabled");
            log_i("Hex: %s", Utils::getHexString(requestData).c_str());
            if (requestValues.find(0x00) != requestValues.end()) {
              currentRequestedPower = requestValues.at(0x00);
              if (!BTDeviceManager::writeFECTargetPower(currentRequestedPower)) {
                log_e("Error writing FEC target power");
              }
            }
            break;
          // Inclination information
          case 0x22:
            if (requestValues.find(0x10) != requestValues.end()) {
              currentInclination = requestValues.at(0x10);
            }
            break;

          // SIM Mode
          case 0x2A:
            if (requestValues.find(0x10) != requestValues.end()) {
              currentGearRatio = requestValues.at(0x10);
              if (currentGearRatio == 0) {
                if (virtualShiftingEnabled) {
                  log_i("Virtual shifting disabled");
                }
                virtualShiftingEnabled = false;
                currentDeviceGearRatio = 0;
                currentDeviceWheelDiameter = 0xFF;
              } else {
                if (!virtualShiftingEnabled) {
                  log_i("Virtual shifting enabled");
                }
                virtualShiftingEnabled = true;
                currentDeviceGearRatio = (uint8_t)(currentGearRatio / 300);
                currentDeviceWheelDiameter = (uint8_t)(currentGearRatio / 340);
              }
            }

            currentDeviceGrade = 0x4e20;
            if (currentGearRatio != 0) {
              currentDeviceGrade = (uint16_t)currentGearRatio;
            }

            if (!BTDeviceManager::writeFECTrackResistance(currentDeviceGrade)) {
              log_e("Error writing FEC track resistance");
            }
            break;

          // Unknown
          default:
            log_e("Unknown Zwift Sync change request with hex value: %s", Utils::getHexString(requestData).c_str());
            for (auto requestValue = requestValues.begin(); requestValue != requestValues.end(); requestValue++) {
              log_i("Zwift sync data key: %d, value %d", requestValue->first, requestValue->second);
            }
            break;
        }

        return true;
        break;

      // Unknown request, similar to 0x00
      case 0x41:
        return true;
        break;

      // RideOn request
      case 0x52:
        if (requestData->size() == 8) {
          sendDirConCharacteristicNotification(NimBLEUUID(ZWIFT_SYNCTX_CHARACTERISTIC_UUID), zwiftSyncRideOnAnswer, sizeof(zwiftSyncRideOnAnswer), false);
          sendDirConCharacteristicNotification(NimBLEUUID(ZWIFT_ASYNC_CHARACTERISTIC_UUID), zwiftAsyncRideOnAnswer, sizeof(zwiftAsyncRideOnAnswer), false);
          return true;
        }
        break;

      // Unknown request
      default:
        log_e("Unknown Zwift Sync request with hex value: %s", Utils::getHexString(requestData).c_str());
        break;
    }
  }
  return false;
}

void DirConManager::notifyDirConCharacteristic(const NimBLEUUID &characteristicUUID, uint8_t *pData, size_t length) {
  notifyDirConCharacteristic(serviceManager->getCharacteristic(characteristicUUID), pData, length);
}

void DirConManager::notifyDirConCharacteristic(Characteristic *characteristic, uint8_t *pData, size_t length) {
  if (characteristic != nullptr) {
    // Fetch current power and cadence data
    if (characteristic->UUID.equals(NimBLEUUID(CYCLING_POWER_MEASUREMENT_CHARACTERISTIC_UUID))) {
      debugMessage = Utils::getHexString(pData, length).c_str();
      uint16_t flags = 0;
      int16_t power = 0;
      uint16_t crankRevolutions = 0;
      uint16_t crankLastEventTime = 0;
      size_t currentIndex = 0;
      if (length >= (currentIndex + 2)) {
        flags = (pData[currentIndex + 1] << 8) | pData[currentIndex];
        currentIndex += 2;
        if (length >= (currentIndex + 2)) {
          power = (pData[currentIndex + 1] << 8) | pData[currentIndex];
          currentDevicePower = power;
          currentIndex += 2;
          if ((flags & 1) == 1) {
            currentIndex += 1;  // Skip "Pedal Power Balance"
          }
          if ((flags & 4) == 4) {
            currentIndex += 2;  // Skip "Accumulated Torque"
          }
          if ((flags & 16) == 16) {
            currentIndex += 6;  // Skip "Wheel Revolution Data"
          }
          if ((flags & 32) == 32) {
            if (length >= (currentIndex + 4)) {
              crankRevolutions = (pData[currentIndex + 1] << 8) | pData[currentIndex];
              currentIndex += 2;
              crankLastEventTime = (pData[currentIndex + 1] << 8) | pData[currentIndex];
              currentIndex += 2;
              if (!currentDeviceCrankStaleness) {
                uint16_t eventTimeDiff = crankLastEventTime - currentDeviceCrankLastEventTime;
                uint16_t revolutionsDiff = crankRevolutions - currentDeviceCrankRevolutions;
                if (eventTimeDiff > 0) {
                  currentDeviceCadence = 1024 * 60 * revolutionsDiff / eventTimeDiff;
                } else {
                  currentDeviceCadence = 0;
                }
              } else {
                currentDeviceCrankStaleness = false;
              }
              currentDeviceCrankRevolutions = crankRevolutions;
              currentDeviceCrankLastEventTime = crankLastEventTime;
            }
          }
        }
      }
    }
    sendDirConCharacteristicNotification(characteristic, pData, length, true);
  }
}

void DirConManager::sendDirConCharacteristicNotification(const NimBLEUUID &characteristicUUID, uint8_t *pData, size_t length, bool onlySubscribers) {
  sendDirConCharacteristicNotification(serviceManager->getCharacteristic(characteristicUUID), pData, length, onlySubscribers);
}

void DirConManager::sendDirConCharacteristicNotification(Characteristic *characteristic, uint8_t *pData, size_t length, bool onlySubscribers) {
  if (onlySubscribers && (characteristic->getSubscriptions().size() == 0)) {
    return;
  }
  DirConMessage dirConMessage;
  dirConMessage.Identifier = DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION;
  dirConMessage.UUID = characteristic->UUID;
  for (size_t dataIndex = 0; dataIndex < length; dataIndex++) {
    dirConMessage.AdditionalData.push_back(pData[dataIndex]);
  }
  std::vector<uint8_t> *messageData = dirConMessage.encode(0);
  for (size_t clientIndex = 0; clientIndex < DIRCON_MAX_CLIENTS; clientIndex++) {
    if ((dirConClients[clientIndex] != nullptr) && dirConClients[clientIndex]->connected()) {
      if ((onlySubscribers && characteristic->isSubscribed(clientIndex)) || !onlySubscribers) {
        dirConClients[clientIndex]->write((char *)messageData->data(), messageData->size());
      }
    }
  }
}

void DirConManager::notifyInternalCharacteristics() {
  for (Service *service : serviceManager->getServices()) {
    if (service->isInternal()) {
      for (Characteristic *characteristic : service->getCharacteristics()) {
        if (characteristic->UUID.equals(NimBLEUUID(ZWIFT_ASYNC_CHARACTERISTIC_UUID))) {
          currentPower = currentDevicePower;
          currentCadence = currentDeviceCadence;
          std::vector<uint8_t> notificationData = generateZwiftAsyncNotificationData(currentPower, currentCadence, 0, 0, 0);
          notifyDirConCharacteristic(characteristic, notificationData.data(), notificationData.size());
        }
      }
    }
  }
}

std::vector<uint8_t> DirConManager::generateZwiftAsyncNotificationData(int64_t power, int64_t cadence, int64_t unknown1, int64_t unknown2, int64_t unknown3, int64_t unknown4) {
  std::vector<uint8_t> notificationData;
  int64_t currentData = 0;
  uint8_t leb128Buffer[16];
  size_t leb128Size = 0;

  notificationData.push_back(0x03);

  for (uint8_t dataBlock = 0x08; dataBlock <= 0x30; dataBlock += 0x08) {
    notificationData.push_back(dataBlock);
    if (dataBlock == 0x08) {
      currentData = power;
    }
    if (dataBlock == 0x10) {
      currentData = cadence;
    }
    if (dataBlock == 0x18) {
      currentData = unknown1;
    }
    if (dataBlock == 0x20) {
      currentData = unknown2;
    }
    if (dataBlock == 0x28) {
      currentData = unknown3;
    }
    if (dataBlock == 0x30) {
      currentData = unknown4;
    }
    leb128Size = bfs::EncodeLeb128(currentData, leb128Buffer, sizeof(leb128Buffer));
    for (uint8_t leb128Byte = 0; leb128Byte < leb128Size; leb128Byte++) {
      notificationData.push_back(leb128Buffer[leb128Byte]);
    }
  }
  return notificationData;
}

int64_t DirConManager::getCurrentPower() {
  return currentPower;
}

int64_t DirConManager::getCurrentCadence() {
  return currentCadence;
}

int64_t DirConManager::getCurrentInclination() {
  return currentInclination;
}

int64_t DirConManager::getCurrentGearRatio() {
  return currentGearRatio;
}

int64_t DirConManager::getCurrentRequestedPower() {
  return currentRequestedPower;
}

bool DirConManager::isVirtualShiftingEnabled() {
  return virtualShiftingEnabled;
}

void DirConManager::setCurrentPower(int64_t power) {
  currentPower = power;
}

void DirConManager::setCurrentCadence(int64_t cadence) {
  currentCadence = cadence;
}

int16_t DirConManager::getCurrentDevicePower() {
  return currentDevicePower;
}

uint16_t DirConManager::getCurrentDeviceCrankRevolutions() {
  return currentDeviceCrankRevolutions;
}

uint16_t DirConManager::getCurrentDeviceCrankLastEventTime() {
  return currentDeviceCrankLastEventTime;
}

uint16_t DirConManager::getCurrentDeviceCadence() {
  return currentDeviceCadence;
}

uint16_t DirConManager::getCurrentDeviceGrade() {
  return currentDeviceGrade;
}
