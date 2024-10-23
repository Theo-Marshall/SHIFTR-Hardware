#include <Arduino.h>
#include <version.h>
#include <Config.h>
#include <utils.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <ETH.h>
#include <arduino-timer.h>
#include <NimBLEDevice.h>
#include <IotWebConf.h>
#include <uuids.h>
#include <tacx.h>
#include <zwift.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <DirConMessage.h>
#include <DirCon.h>
#include <DirConService.h>

void subscribeBLENotification(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID);
void unSubscribeBLENotification(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID);
std::vector<uint8_t> readBLECharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID);
bool writeBLECharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID, std::vector<uint8_t> data);
static void bLENotifyCallback(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
std::vector<uint8_t> readDirConCharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID);
bool writeDirConCharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID, std::vector<uint8_t> data);
void doDirConLoop();
bool writeDirConMessage(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID, DirConMessage message, uint8_t sequenceNumber);
bool notifyDirConClients(void *argument);
uint8_t getDirConClientIndex(AsyncClient *client);
void handleDirConData(void *arg, AsyncClient *client, void *data, size_t len);
void handleDirConError(void *arg, AsyncClient *client, int8_t error);
void handleDirConDisconnect(void *arg, AsyncClient *client);
void handleDirConTimeOut(void *arg, AsyncClient *client, uint32_t time);
void handleNewDirConClient(void *arg, AsyncClient *client);
void initializeBLE();
void doBLELoop();
void connectBLETrainerDevice();
void initializeEthernet();
void initializeWiFiManager();
void WiFiEvent(WiFiEvent_t event);
void handleWebServerRoot();

const char *macAddressString = getMacAddressString();
const char *serialNumberString = getSerialNumberString();
const char *deviceName = getDeviceName();
const char *hostName = getHostName();

bool mDNSStarted = false;
bool bLEConnected = false;
bool ethernetConnected = false;
bool wiFiConnected = false;

auto timer = timer_create_default();

int64_t currentPower = 150;
int64_t currentCadence = 75;

AsyncServer *dirConServer = new AsyncServer(DIRCON_TCP_PORT);
AsyncClient *dirConClients[DIRCON_MAX_CLIENTS];
std::vector<DirConService> dirConServices;

DNSServer dnsServer;
WebServer webServer(WEB_SERVER_PORT);

IotWebConf iotWebConf(hostName, &dnsServer, &webServer, hostName, WIFI_CONFIG_VERSION);

NimBLEScan *bLEScanner;
std::vector<NimBLEAdvertisedDevice> trainerDevices;
size_t selectedTrainerDeviceIndex;
NimBLEClient *bLEClient;

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{

  void onResult(NimBLEAdvertisedDevice *advertisedDevice)
  {
    log_d("Advertised BLE device found: %s", advertisedDevice->toString().c_str());
    if (advertisedDevice->isAdvertisingService(cyclingPowerServiceUUID))
    {
      log_d("Device %s offers cycling power service %s", advertisedDevice->getName().c_str(), cyclingPowerServiceUUID.toString().c_str());
      if (advertisedDevice->isAdvertisingService(cyclingSpeedAndCadenceServiceUUID))
      {
        log_d("Device %s offers cycling speed and cadence service %s", advertisedDevice->getName().c_str(), cyclingSpeedAndCadenceServiceUUID.toString().c_str());
        log_d("Adding %s to device list", advertisedDevice->getName().c_str());
        trainerDevices.push_back(NimBLEAdvertisedDevice(*advertisedDevice));

        log_d("Number of advertised services: %d", advertisedDevice->getServiceUUIDCount());
        for (size_t index = 0; index < advertisedDevice->getServiceUUIDCount(); index++)
        {
          log_d("Advertised service: %s", advertisedDevice->getServiceUUID(index).to128().toString().c_str());
        }
        
      }
    }
  };
};

class ClientCallbacks : public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient *pClient)
  {
    log_d("BLE client connected");
    bLEConnected = true;
  };

  void onDisconnect(NimBLEClient *pClient)
  {
    log_d("BLE client disconnected");
    bLEConnected = false;
  };
};

static void bLENotifyCallback(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
  log_d("Notification for BLE characteristic %s received, notify %d, hex value: %s, forwarding...", pRemoteCharacteristic->getUUID().to128().toString().c_str(), isNotify, getHexString(pData, length).c_str());
  std::vector<uint8_t> returnData;
  for (size_t index = 0; index < length; index++)
  {
    returnData.push_back(pData[index]);
  }
  
  DirConMessage returnMessage;
  returnMessage.Identifier = DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION;
  returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
  returnMessage.UUID = pRemoteCharacteristic->getUUID();
  returnMessage.AdditionalData = returnData;
  if (!writeDirConMessage(pRemoteCharacteristic->getRemoteService()->getUUID(), pRemoteCharacteristic->getUUID(), returnMessage, 0))
  {
    log_e("Error forwarding BLE notification to DirCon clients");
    pRemoteCharacteristic->unsubscribe();
  }
 
}

std::vector<uint8_t> readBLECharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID)
{
  log_i("Reading BLE service %s with characteristic %s...", serviceUUID.to128().toString().c_str(), characteristicUUID.to128().toString().c_str());
  std::vector<uint8_t> returnValue;

  if (bLEClient->isConnected())
  {
    NimBLERemoteService* remoteService = nullptr;
    NimBLERemoteCharacteristic* remoteCharacteristic = nullptr;
    remoteService = bLEClient->getService(serviceUUID);
    if (remoteService)
    {
      remoteCharacteristic = remoteService->getCharacteristic(characteristicUUID);
      if (remoteCharacteristic)
      {
        if (remoteCharacteristic->canRead())
        {
          returnValue = remoteCharacteristic->readValue();
        } else
        {
          log_e("Reading failed: BLE characteristic doesn't feature read");
        }
      } else
      {
        log_e("Reading failed: BLE characteristic not found");
      }
    } else
    {
      log_e("Reading failed: BLE service not found");
    }
  } else
  {
    log_e("Reading failed: BLE client not connected");
  }
  return returnValue;
}

bool writeBLECharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID, std::vector<uint8_t> data)
{
  log_i("Writing BLE service %s with characteristic %s...", serviceUUID.to128().toString().c_str(), characteristicUUID.to128().toString().c_str());
  bool success = false;
  
  if (bLEClient->isConnected())
  {
    NimBLERemoteService* remoteService = nullptr;
    NimBLERemoteCharacteristic* remoteCharacteristic = nullptr;
    remoteService = bLEClient->getService(serviceUUID);
    if (remoteService)
    {
      remoteCharacteristic = remoteService->getCharacteristic(characteristicUUID);
      if (remoteCharacteristic)
      {
        if (remoteCharacteristic->canWrite())
        {
          success = remoteCharacteristic->writeValue(data.data());
        } else
        {
          log_e("Writing failed: BLE characteristic doesn't feature write");
        }
      } else
      {
        log_e("Writing failed: BLE characteristic not found");
      }
    } else
    {
      log_e("Writing failed: BLE service not found");
    }
  } else
  {
    log_e("Writing failed: BLE client not connected");
  }
  return success;
}

void subscribeBLENotification(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID)
{
  log_i("Subscribing to BLE service %s with characteristic %s...", serviceUUID.to128().toString().c_str(), characteristicUUID.to128().toString().c_str());
  if (bLEClient->isConnected())
  {
    NimBLERemoteService* remoteService = nullptr;
    NimBLERemoteCharacteristic* remoteCharacteristic = nullptr;
    remoteService = bLEClient->getService(serviceUUID);
    if (remoteService)
    {
      remoteCharacteristic = remoteService->getCharacteristic(characteristicUUID);
      if (remoteCharacteristic)
      {
        if (remoteCharacteristic->canNotify())
        {
          if (!remoteCharacteristic->subscribe(true, bLENotifyCallback))
          {
            log_e("Subscription failed: Subscription notify call returned false");
          }
        } 
        if (remoteCharacteristic->canIndicate())
        {
          if (!remoteCharacteristic->subscribe(false, bLENotifyCallback))
          {
            log_e("Subscription failed: Subscription indicate call returned false");
          }
        } 
        if (!(remoteCharacteristic->canNotify() || remoteCharacteristic->canIndicate()))
        {
          log_e("Subscription failed: BLE characteristic doesn't feature notify or indicate");
        }
      } else
      {
        log_e("Subscription failed: BLE characteristic not found");
      }
    } else
    {
      log_e("Subscription failed: BLE service not found");
    }
  } else
  {
    log_e("Subscription failed: BLE client not connected");
  }
}

void unSubscribeBLENotification(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID)
{
  log_i("Unsubscribing from BLE service %s with characteristic %s...", serviceUUID.to128().toString().c_str(), characteristicUUID.to128().toString().c_str());
  if (bLEClient->isConnected())
  {
    NimBLERemoteService* remoteService = nullptr;
    NimBLERemoteCharacteristic* remoteCharacteristic = nullptr;
    remoteService = bLEClient->getService(serviceUUID);
    if (remoteService)
    {
      remoteCharacteristic = remoteService->getCharacteristic(characteristicUUID);
      if (remoteCharacteristic)
      {
        if (remoteCharacteristic->canNotify() || remoteCharacteristic->canIndicate())
        {
          if (!remoteCharacteristic->unsubscribe())
          {
            log_e("Unsubscribe failed: Unsubscribe call returned false");
          }
        } else
        {
          log_e("Unsubscribe failed: BLE characteristic doesn't feature notify or indicate");
        }
      } else
      {
        log_e("Unsubscribe failed: BLE characteristic not found");
      }
    } else
    {
      log_e("Unsubscribe failed: BLE service not found");
    }
  } else
  {
    log_e("Unsubscribe failed: BLE client not connected");
  }
}

std::vector<uint8_t> readDirConCharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID)
{
  std::vector<uint8_t> returnValue;
  return returnValue;
}

bool writeDirConCharacteristic(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID, std::vector<uint8_t> data)
{
  return true;
}

void setup()
{
  log_i("SHIFTR " VERSION " starting...");
  initializeEthernet();
  initializeWiFiManager();
  initializeBLE();
}

void loop()
{
  iotWebConf.doLoop();
  doBLELoop();
  doDirConLoop();
  timer.tick();
}

void doDirConLoop()
{
  if ((wiFiConnected || ethernetConnected) && bLEConnected)
  {
    if (!mDNSStarted)
    {
      log_d("Ethernet or WiFi and BLE connected, starting MDNS...");
      if (!MDNS.begin(getHostName()))
      {
        log_d("Unable to start MDNS for %s.local", getHostName());
      }
      else
      {
        mDNSStarted = true;
        log_d("MDNS resolving to %s.local", getHostName());
        MDNS.setInstanceName(getDeviceName());
        MDNS.addService(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, DIRCON_TCP_PORT);
        MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "mac-address", getMacAddressString());
        MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "serial-number", getSerialNumberString());
        String serviceUUIDs = "";
        for (size_t index = 0; index < dirConServices.size(); index++)
        {
          if (dirConServices.at(index).Advertised)
          {
            serviceUUIDs += dirConServices.at(index).UUID.to16().toString().c_str();
            serviceUUIDs += ",";
          }
        }
        if (serviceUUIDs.endsWith(","))
        {
          serviceUUIDs.remove(serviceUUIDs.length() - 1);
        }
        log_d("Service UUIDs: %s", serviceUUIDs.c_str());
        MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "ble-service-uuids", serviceUUIDs.c_str());
        //MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "ble-service-uuids", "0x1816,0x1818,0xa305,0xfec1");
        log_d("Added MDNS serviceTxts");
        log_d("Starting DirCon server...");
        dirConServer->begin();
        dirConServer->onClient(&handleNewDirConClient, dirConServer);
        log_d("Starting DirCon notification timer...");
        timer.every(1000, notifyDirConClients);
      }
    }
  }
  else
  {
    if (mDNSStarted)
    {
      log_d("Ethernet and WiFi or BLE disconnected, stopping MDNS...");
      mDNSStarted = false;
      MDNS.end();
      log_d("Stopping DirCon notification timer...");
      timer.cancel();
      log_d("Stopping DirCon server...");
      dirConServer->end();
      log_d("Clearing DirCon services...");
      dirConServices.clear();
    }
  }
}

bool writeDirConMessage(NimBLEUUID serviceUUID, NimBLEUUID characteristicUUID, DirConMessage message, uint8_t sequenceNumber)
{
  bool success = false;

  if (wiFiConnected || ethernetConnected)
  {
    if (dirConServer != nullptr)
    {
      for (size_t serviceIndex = 0; serviceIndex < dirConServices.size(); serviceIndex++)
      {
        if (dirConServices.at(serviceIndex).UUID.equals(serviceUUID)) 
        {
          for (size_t characteristicIndex = 0; characteristicIndex < dirConServices.at(serviceIndex).Characteristics.size(); characteristicIndex++)
          {
            if (dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).NotificationSubscriptions.size() > 0) 
            {
              if (dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID.equals(characteristicUUID))
              {
                for (size_t subscriptionIndex = 0; subscriptionIndex < dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).NotificationSubscriptions.size(); subscriptionIndex++)
                {
                  if ((dirConClients[dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).NotificationSubscriptions.at(subscriptionIndex)] != nullptr) && dirConClients[dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).NotificationSubscriptions.at(subscriptionIndex)]->connected())
                  {
                    log_d("Writing DirCon message to client #%d for characteristic %s", dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).NotificationSubscriptions.at(subscriptionIndex), dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID.to128().toString().c_str());
                    std::vector<uint8_t> clientData = message.encode(sequenceNumber);
                    success = dirConClients[dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).NotificationSubscriptions.at(subscriptionIndex)]->write((char*)clientData.data(), clientData.size());
                    if (!success)
                    {
                      log_e("Error writing DirCon message");
                    }
                  }
                }
                break;
              }
            }
          }
          break;
        }
      }
    }
  }
  return success;
}

bool notifyDirConClients(void *argument)
{
  //log_d("Notifying DirCon clients");
  //DirConMessage returnMessage;
  //returnMessage.Identifier = DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION;
  //returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
  //returnMessage.UUID = zwiftAsyncCharacteristicUUID;
  //returnMessage.AdditionalData = generateZwiftAsyncNotificationData(currentPower, currentCadence, 0LL, 0LL, 0LL);
  //writeDirConMessage(zwiftCustomServiceUUID, zwiftAsyncCharacteristicUUID, returnMessage);
  return true;
}

uint8_t getDirConClientIndex(AsyncClient *client)
{
  uint8_t clientIndex = 0xFF;
  for (size_t index = 0; index < DIRCON_MAX_CLIENTS; index++)
  {
    if (dirConClients[index] != nullptr)
    {
      if (dirConClients[index] == client) 
      {
        clientIndex = index;
        break;
      } 
    }
  }
  return clientIndex;
}

void handleDirConData(void *arg, AsyncClient *client, void *data, size_t len)
{
  uint8_t clientIndex = getDirConClientIndex(client);
  if (clientIndex == 0xFF) 
  {
    log_e("Unable to identify client index, skipping handling of data");
    return;
  } 

  //uint8_t *clientData = (uint8_t *)data;
  //log_d("Data from DirCon client #%d with IP %s received, hex value: %s", clientIndex, client->remoteIP().toString().c_str(), getHexString(clientData, len).c_str());
  
  DirConMessage inputMessage;
  if (!inputMessage.parse((uint8_t*)data, len, 0))
  {
    log_e("Failed to parse DirCon message, skipping further processing");
    return;
  }
  
  std::vector<uint8_t> returnData;
  DirConMessage returnMessage;
  returnMessage.Identifier = inputMessage.Identifier;
  bool uUIDFound = false;
  bool enableNotification = false;
  switch (inputMessage.Identifier)
  {
    case DIRCON_MSGID_DISCOVER_SERVICES:
      log_i("DirCon service discovery request, returning services");
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      for (size_t serviceIndex = 0; serviceIndex < dirConServices.size(); serviceIndex++)
      {
        if (dirConServices.at(serviceIndex).Advertised)
        {
          returnMessage.AdditionalUUIDs.push_back(dirConServices.at(serviceIndex).UUID);
        }
      }
      break;
    case DIRCON_MSGID_DISCOVER_CHARACTERISTICS:
      log_i("DirCon characteristic discovery request for service UUID %s, returning characteristics", inputMessage.UUID.to128().toString().c_str());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      returnMessage.UUID = inputMessage.UUID;
      uUIDFound = false;
      for (size_t serviceIndex = 0; serviceIndex < dirConServices.size(); serviceIndex++)
      {
        if (dirConServices.at(serviceIndex).UUID.equals(inputMessage.UUID))
        {
          for (size_t characteristicIndex = 0; characteristicIndex < dirConServices.at(serviceIndex).Characteristics.size(); characteristicIndex++)
          {
            log_d("Returning DirCon characteristic %s with type %d", dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID.to128().toString().c_str(), dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).Type);
            returnMessage.AdditionalUUIDs.push_back(dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID);
            returnMessage.AdditionalData.push_back(dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).Type);
          }
          uUIDFound = true;
          break;
        }
      }
      if (!uUIDFound)
      {
        log_e("Unknown service UUID %s for characteristic discovery requested, skipping further processing", inputMessage.UUID.to128().toString().c_str());
        returnMessage.ResponseCode = DIRCON_RESPCODE_SERVICE_NOT_FOUND;
      }
      break;      
    case DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS:
      log_i("DirCon characteristic enable notification request for characteristic %s with additional data of %d bytes", inputMessage.UUID.to128().toString().c_str(), inputMessage.AdditionalData.size());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      returnMessage.UUID = inputMessage.UUID;
      if (inputMessage.AdditionalData.size() == 1) {
        enableNotification = (inputMessage.AdditionalData.at(0) != 0);
      } else 
      {
        log_e("Value of enable notification request is missing, skipping further processing", inputMessage.Identifier);
        returnMessage.Identifier = DIRCON_RESPCODE_CHARACTERISTIC_OPERATION_NOT_SUPPORTED;
      }
      uUIDFound = false;
      for (size_t serviceIndex = 0; serviceIndex < dirConServices.size(); serviceIndex++)
      {
        for (size_t characteristicIndex = 0; characteristicIndex < dirConServices.at(serviceIndex).Characteristics.size(); characteristicIndex++)
        {
          if (dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID.equals(inputMessage.UUID)) 
          {
            log_d("Subscribing for characteristic %s with value %d for clientIndex %d", dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID.to128().toString().c_str(), enableNotification, clientIndex);
            dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).subscribeNotification(clientIndex, enableNotification);
            // if there's at least one subscriber then enable BLE notifications, otherwise disable them (except Zwift special service)
            if (!dirConServices.at(serviceIndex).UUID.equals(zwiftCustomServiceUUID))
            {
              if (dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).NotificationSubscriptions.size() > 0) 
              {
                subscribeBLENotification(dirConServices.at(serviceIndex).UUID, dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID);
              } else 
              {
                unSubscribeBLENotification(dirConServices.at(serviceIndex).UUID, dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID);
              }
            }
            uUIDFound = true;
            break;
          }
        }
        if (uUIDFound)
        {
          break;
        }
      }
      if (!uUIDFound)
      {
        log_e("Unknown characteristic UUID %s for enable notification requested, skipping further processing", inputMessage.UUID.to128().toString().c_str());
        returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_NOT_FOUND;
      }
      break;
    case DIRCON_MSGID_READ_CHARACTERISTIC:
      log_i("DirCon read characteristic request for characteristic %s", inputMessage.UUID.to128().toString().c_str());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      returnMessage.UUID = inputMessage.UUID;
      uUIDFound = false;
      for (size_t serviceIndex = 0; serviceIndex < dirConServices.size(); serviceIndex++)
      {
        for (size_t characteristicIndex = 0; characteristicIndex < dirConServices.at(serviceIndex).Characteristics.size(); characteristicIndex++)
        {
          if (dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID.equals(inputMessage.UUID)) 
          {
            if (!dirConServices.at(serviceIndex).UUID.equals(zwiftCustomServiceUUID))
            {
              returnMessage.AdditionalData = readBLECharacteristic(dirConServices.at(serviceIndex).UUID, dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID);
            } else
            {
              returnMessage.AdditionalData = readDirConCharacteristic(dirConServices.at(serviceIndex).UUID, dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID);
            }
            uUIDFound = true;
            break;
          }
        }
        if (uUIDFound)
        {
          break;
        }
      }
      if (!uUIDFound)
      {
        log_e("Unknown characteristic UUID %s for read requested, skipping further processing", inputMessage.UUID.to128().toString().c_str());
        returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_NOT_FOUND;
      }
      break;
    case DIRCON_MSGID_WRITE_CHARACTERISTIC:
      log_i("DirCon write characteristic request for characteristic %s", inputMessage.UUID.to128().toString().c_str());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      returnMessage.UUID = inputMessage.UUID;
      uUIDFound = false;
      for (size_t serviceIndex = 0; serviceIndex < dirConServices.size(); serviceIndex++)
      {
        for (size_t characteristicIndex = 0; characteristicIndex < dirConServices.at(serviceIndex).Characteristics.size(); characteristicIndex++)
        {
          if (dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID.equals(inputMessage.UUID)) 
          {
            if (!dirConServices.at(serviceIndex).UUID.equals(zwiftCustomServiceUUID))
            {
              if (!writeBLECharacteristic(dirConServices.at(serviceIndex).UUID, dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID, inputMessage.AdditionalData))
              {
                returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_OPERATION_NOT_SUPPORTED;
              }
            } else
            {
              if (!writeDirConCharacteristic(dirConServices.at(serviceIndex).UUID, dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID, inputMessage.AdditionalData))
              {
                returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_OPERATION_NOT_SUPPORTED;
              }
            }
            uUIDFound = true;
            break;
          }
        }
        if (uUIDFound)
        {
          break;
        }
      }
      if (!uUIDFound)
      {
        log_e("Unknown characteristic UUID %s for write requested, skipping further processing", inputMessage.UUID.to128().toString().c_str());
        returnMessage.ResponseCode = DIRCON_RESPCODE_CHARACTERISTIC_NOT_FOUND;
      }
      break;
    default:
      log_e("Unknown identifier %d in DirCon message, skipping further processing", inputMessage.Identifier);
      returnMessage.Identifier = DIRCON_MSGID_ERROR;
      break;
  }

  returnData = returnMessage.encode(inputMessage.SequenceNumber);
  if (returnData.size() > 0) 
  {
    //log_d("Sending data to DirCon client #%d with IP %s, hex value: %s", clientIndex, client->remoteIP().toString().c_str(), getHexString(returnData.data(), returnData.size()).c_str());
    client->write((char*)returnData.data(), returnData.size());
  } else
  {
    log_e("DirCon message parsed but no data to answer");
  }
}

void handleDirConError(void *arg, AsyncClient *client, int8_t error)
{
  log_d("DirCon client connection error %s from %s", client->errorToString(error), client->remoteIP().toString().c_str());
  client->stop();
}

void handleDirConDisconnect(void *arg, AsyncClient *client)
{
  log_d("DirCon client disconnected");
  uint8_t clientIndex = getDirConClientIndex(client);
  
  if (clientIndex == 0xFF) 
  {
    log_e("Unable to identify client index, removing subscriptions failed");
    return;
  } 

  for (size_t index = 0; index < dirConServices.size(); index++)
  {
    dirConServices.at(index).unSubscribeNotifications(clientIndex);
  }

}

void handleDirConTimeOut(void *arg, AsyncClient *client, uint32_t time)
{
  log_d("DirCon client ACK timeout from %s", client->remoteIP().toString().c_str());
  client->stop();
}

void handleNewDirConClient(void *arg, AsyncClient *client)
{
  bool clientAccepted = false;
  log_d("New DirCon connection from %s", client->remoteIP().toString().c_str());
  for (size_t counter = 0; counter < DIRCON_MAX_CLIENTS; counter++)
  {
    if ((dirConClients[counter] == nullptr) || ((dirConClients[counter] != nullptr) && (!dirConClients[counter]->connected())))
    {
      dirConClients[counter] = client;
      clientAccepted = true;
      break;
    }
  }
  if (clientAccepted)
  {
    log_d("Free connection slot found, DirCon connection from %s accepted", client->remoteIP().toString().c_str());
    client->onData(&handleDirConData, NULL);
    client->onError(&handleDirConError, NULL);
    client->onDisconnect(&handleDirConDisconnect, NULL);
    client->onTimeout(&handleDirConTimeOut, NULL);
  }
  else
  {
    log_d("Maximum number of %d connections reached, DirCon connection from %s rejected", DIRCON_MAX_CLIENTS, client->remoteIP().toString().c_str());
  }
}

void initializeBLE()
{
  log_d("Starting BLE...");
  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE);
  NimBLEDevice::setScanDuplicateCacheSize(200);

  NimBLEDevice::init(deviceName);

  bLEScanner = NimBLEDevice::getScan(); // create new scan
  // Set the callback for when devices are discovered, no duplicates.
  bLEScanner->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks(), false);
  bLEScanner->setActiveScan(true); // Set active scanning, this will get more data from the advertiser.
  bLEScanner->setInterval(97);     // How often the scan occurs / switches channels; in milliseconds,
  bLEScanner->setWindow(37);       // How long to scan during the interval; in milliseconds.
  bLEScanner->setMaxResults(0);    // do not store the scan results, use callback only.
  log_d("BLE started");
}

void doBLELoop()
{
  if (!bLEConnected)
  {
    if (bLEScanner->isScanning() == false)
    {
      log_d("Starting BLE scan");
      trainerDevices.clear();
      selectedTrainerDeviceIndex = -1;
      bLEScanner->start(0, nullptr, false);
    }
    if (trainerDevices.size() > 0)
    {
      for (size_t counter = 0; counter < trainerDevices.size(); counter++)
      {
        if (trainerDevices[counter].getName().rfind(BLE_DEVICE_NAME_PREFIX, 0) == 0)
        {
          log_d("Stopping BLE scan");
          NimBLEDevice::getScan()->stop();
          log_d("Matching device %s will be connected...", trainerDevices[counter].getName().c_str());
          selectedTrainerDeviceIndex = counter;
          connectBLETrainerDevice();
          break;
        }
      }
    }
  }
}

void connectBLETrainerDevice()
{
  if (trainerDevices.size() > 0)
  {
    bLEClient = nullptr;
    bLEClient = NimBLEDevice::createClient();
    bLEClient->setClientCallbacks(new ClientCallbacks, false);

    /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
    bLEClient->setConnectTimeout(10);

    if (!bLEClient->connect(&trainerDevices[selectedTrainerDeviceIndex]))
    {
      /* Created a client but failed to connect, don't need to keep it as it has no data */
      NimBLEDevice::deleteClient(bLEClient);
      log_e("Failed to connect to %s, deleting client", trainerDevices[selectedTrainerDeviceIndex].getName().c_str());
      bLEConnected = false;
    }
    else
    {
      bLEConnected = true;
    }

    log_d("Connected to %s with RSSI %d", bLEClient->getPeerAddress().toString().c_str(), bLEClient->getRssi());

    log_d("Clearing DirCon services and adding Zwift default service");
    dirConServices.clear();
    DirConService dirConZwiftService(zwiftCustomServiceUUID);
    dirConZwiftService.Advertised = true;
    dirConZwiftService.addCharacteristic(zwiftAsyncCharacteristicUUID, DIRCON_CHAR_PROP_FLAG_NOTIFY);
    dirConZwiftService.addCharacteristic(zwiftSyncRXCharacteristicUUID, DIRCON_CHAR_PROP_FLAG_WRITE);
    dirConZwiftService.addCharacteristic(zwiftSyncTXCharacteristicUUID, DIRCON_CHAR_PROP_FLAG_READ);
    //dirConServices.push_back(dirConZwiftService);

    log_d("Iterating remote BLE services and characteristics...");
    std::vector<NimBLERemoteService *> *remoteServices = bLEClient->getServices(true);
    for (size_t serviceIndex = 0; serviceIndex < remoteServices->size(); serviceIndex++)
    {
      log_d("Found service %s, reading characteristics...", remoteServices->at(serviceIndex)->getUUID().to128().toString().c_str());
      DirConService dirConService(remoteServices->at(serviceIndex)->getUUID());
      if (trainerDevices[selectedTrainerDeviceIndex].isAdvertisingService(remoteServices->at(serviceIndex)->getUUID()))
      {
        log_d("Service %s is advertised", remoteServices->at(serviceIndex)->getUUID().to128().toString().c_str());
        dirConService.Advertised = true;
      }
      if (tacxCustomFECServiceUUID.equals(remoteServices->at(serviceIndex)->getUUID()) || tacxCustomServiceUUID.equals(remoteServices->at(serviceIndex)->getUUID())) 
      {
        log_d("Service %s is force advertised", remoteServices->at(serviceIndex)->getUUID().to128().toString().c_str());
        dirConService.Advertised = true;
      }
      std::vector<NimBLERemoteCharacteristic *> *remoteCharacteristics = remoteServices->at(serviceIndex)->getCharacteristics(true);
      for (size_t characteristicIndex = 0; characteristicIndex < remoteCharacteristics->size(); characteristicIndex++)
      {
        log_d("Found characteristic %s with type %d", remoteCharacteristics->at(characteristicIndex)->getUUID().to128().toString().c_str(), getDirConCharacteristicTypeFromBLEProperties(remoteCharacteristics->at(characteristicIndex)));
        dirConService.addCharacteristic(remoteCharacteristics->at(characteristicIndex)->getUUID(), getDirConCharacteristicTypeFromBLEProperties(remoteCharacteristics->at(characteristicIndex)));
      }
      dirConServices.push_back(dirConService);
    }
  }
  else
  {
    bLEConnected = false;
  }
}

void initializeEthernet()
{
  log_d("Starting ethernet...");
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
}

void initializeWiFiManager()
{
  log_d("Starting wifi...");
  iotWebConf.setStatusPin(WIFI_STATUS_PIN);
  iotWebConf.setConfigPin(WIFI_CONFIG_PIN);
  iotWebConf.init();
  webServer.on("/", handleWebServerRoot);
  webServer.on("/config", []
               { iotWebConf.handleConfig(); });
  webServer.onNotFound([]()
                       { iotWebConf.handleNotFound(); });
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    log_d("Ethernet started");
    log_d("Setting hostname to %s", hostName);
    ETH.setHostname(hostName);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    log_d("Ethernet connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    log_d("Ethernet DHCP successful");
    log_d("DHCP successful with IP %u.%u.%u.%u\r\n", ETH.localIP()[0], ETH.localIP()[1], ETH.localIP()[2], ETH.localIP()[3]);
    ethernetConnected = true;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    log_d("Ethernet disconnected");
    ethernetConnected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    log_d("Ethernet stopped");
    ethernetConnected = false;
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    log_d("WiFi DHCP successful");
    log_d("DHCP successful with IP %u.%u.%u.%u\r\n", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    wiFiConnected = true;
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    log_d("WiFi disconnected");
    wiFiConnected = false;
    break;
  case ARDUINO_EVENT_WIFI_STA_STOP:
    log_d("WiFi stopped");
    wiFiConnected = false;
    break;

  default:
    break;
  }
}

void handleWebServerRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>";
  s += deviceName;
  s += "</title></head><body>";
  s += "<h1>";
  s += deviceName;
  s += "</h1>";
  s += "<p>Ethernet: ";
  if (ethernetConnected)
  {
    s += "Connected";
    s += ", IP: ";
    s += ETH.localIP().toString();
  }
  else
  {
    s += "Not connected";
  }
  s += "</p>";
  s += "<p>WiFi: ";
  switch (iotWebConf.getState())
  {
  case iotwebconf::NetworkState::ApMode:
    s += "Access-Point mode";
    break;
  case iotwebconf::NetworkState::Boot:
    s += "Booting";
    break;
  case iotwebconf::NetworkState::Connecting:
    s += "Connecting";
    break;
  case iotwebconf::NetworkState::NotConfigured:
    s += "Not configured";
    break;
  case iotwebconf::NetworkState::OffLine:
    s += "Offline";
    break;
  case iotwebconf::NetworkState::OnLine:
    s += "Online";
    s += ", SSID: ";
    s += WiFi.SSID();
    s += ", IP: ";
    s += WiFi.localIP().toString();
    break;
  default:
    s += "Unknown";
    break;
  }
  s += "</p>";
  s += "<p>BLE: ";
  if (bLEConnected)
  {
    s += "Connected";
    s += ", device: ";
    s += trainerDevices[selectedTrainerDeviceIndex].getName().c_str();
  }
  else
  {
    s += "Not connected";
  }
  s += "</p>";
  s += "Go to <a href='config'>configuration</a> to change settings.";
  s += "</body></html>\n";

  webServer.send(200, "text/html", s);
}