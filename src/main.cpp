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

void doDirConLoop();
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

int64_t currentPower = 150;
int64_t currentCadence = 75;

AsyncServer *dirConServer = new AsyncServer(DIRCON_TCP_PORT);
AsyncClient *dirConClients[DIRCON_MAX_CLIENTS];
DirConMessage dirConMessage[DIRCON_MAX_CLIENTS];
uint8_t dirConLastSequenceNumber[DIRCON_MAX_CLIENTS];
std::vector<DirConService> dirConServices;

DNSServer dnsServer;
WebServer webServer(WEB_SERVER_PORT);

IotWebConf iotWebConf(hostName, &dnsServer, &webServer, hostName, WIFI_CONFIG_VERSION);

NimBLEScan *bLEScanner;
std::vector<NimBLEAdvertisedDevice> trainerDevices;
size_t selectedTrainerDeviceIndex;
NimBLEClient *bLEClient;
//NimBLERemoteService *bLECyclingPowerService;
//NimBLERemoteService *bLECyclingSpeedAndCadenceService;
//std::vector<NimBLERemoteCharacteristic*> *bLECyclingPowerServiceCharacteristics;
//std::vector<NimBLERemoteCharacteristic*> *bLECyclingSpeedAndCadenceServiceCharacteristics;

/* Define a class to handle the callbacks when advertisements are received */
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
      }
    }
  };
};

/* Define a class to handle the callbacks for client connection events */
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

void setup()
{
  log_i("SHIFTR " VERSION " starting...");
  /*
  std::vector<uint8_t> messageData;
  std::vector<uint8_t> dirConMessage;
  
  NimBLEUUID uuid1("1818");
  NimBLEUUID uuid2("1816");

  log_d("UUID 0 before: %s", uuid1.to128().toString().c_str());
  log_d("UUID 1 before: %s", uuid2.to128().toString().c_str());

  uint16_t length = 0;
  uint8_t *uuidBytes1 = (uint8_t*)uuid1.to128().getNative();
  uint8_t *uuidBytes2 = (uint8_t*)uuid2.to128().getNative();

  dirConMessage.push_back(1);
  dirConMessage.push_back(DIRCON_MSGID_DISCOVER_SERVICES);
  dirConMessage.push_back(0);
  dirConMessage.push_back(DIRCON_RESPCODE_SUCCESS_REQUEST);
  length += 32;
  dirConMessage.push_back((uint8_t)(length >> 8));
  dirConMessage.push_back((uint8_t)(length));

  for (uint8_t i = 16; i > 0; i--)
  {
    dirConMessage.push_back(uuidBytes1[i]);
  }
  for (uint8_t i = 16; i > 0; i--)
  {
    dirConMessage.push_back(uuidBytes2[i]);
  }

  DirConMessage message;
  message.parse(dirConMessage.data(), dirConMessage.size(), 0);
  log_d("UUID 0 after: %s", message.AdditionalUUIDs[0].toString().c_str());
  log_d("UUID 1 after: %s", message.AdditionalUUIDs[1].toString().c_str());
  */
  initializeEthernet();
  initializeWiFiManager();
  initializeBLE();
}

void loop()
{
  iotWebConf.doLoop();
  doBLELoop();
  doDirConLoop();
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
          serviceUUIDs += dirConServices.at(index).UUID.to128().toString().c_str();
          if (index < (dirConServices.size() -1)) 
          {
            serviceUUIDs += ",";
          }
        }
        MDNS.addServiceTxt(DIRCON_MDNS_SERVICE_NAME, DIRCON_MDNS_SERVICE_PROTOCOL, "ble-service-uuids", serviceUUIDs);
        log_d("Added MDNS serviceTxts");
        log_d("Starting DirCon server...");
        dirConServer->begin();
        dirConServer->onClient(&handleNewDirConClient, dirConServer);
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
      log_d("Stopping DirCon server...");
      dirConServer->end();
      log_d("Clearing DirCon services...");
      dirConServices.clear();
    }
  }
}

void handleDirConData(void *arg, AsyncClient *client, void *data, size_t len)
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

  if (clientIndex == 0xFF) 
  {
    log_e("Unable to identify client index, skipping handling of data");
    return;
  } 

  uint8_t *clientData = (uint8_t *)data;
  log_d("Data from DirCon client #%d with IP %s received, hex value: %s", clientIndex, client->remoteIP().toString().c_str(), getHexString(clientData, len).c_str());
  
  dirConLastSequenceNumber[clientIndex] = dirConMessage[clientIndex].SequenceNumber;
  if (!dirConMessage[clientIndex].parse((uint8_t*)data, len, dirConLastSequenceNumber[clientIndex])) 
  {
    log_e("Failed to parse DirCon message, skipping further processing");
    return;
  }

  std::vector<uint8_t> returnData;
  DirConMessage returnMessage;
  returnMessage.Identifier = dirConMessage[clientIndex].Identifier;
  bool serviceFound = false;
  switch (dirConMessage[clientIndex].Identifier)
  {
    case DIRCON_MSGID_DISCOVER_SERVICES:
      log_i("DirCon service discovery request, returning services");
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      for (size_t serviceIndex = 0; serviceIndex < dirConServices.size(); serviceIndex++)
      {
        returnMessage.AdditionalUUIDs.push_back(dirConServices.at(serviceIndex).UUID);
      }
      break;
    case DIRCON_MSGID_DISCOVER_CHARACTERISTICS:
      log_i("DirCon characteristic discovery request for service UUID %s, returning characteristics", dirConMessage[clientIndex].UUID.to128().toString().c_str());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      for (size_t serviceIndex = 0; serviceIndex < dirConServices.size(); serviceIndex++)
      {
        if (dirConServices.at(serviceIndex).UUID.equals(dirConMessage[clientIndex].UUID))
        {
          returnMessage.UUID = dirConMessage[clientIndex].UUID;
          for (size_t characteristicIndex = 0; characteristicIndex < dirConServices.at(serviceIndex).Characteristics.size(); characteristicIndex++)
          {
            log_d("Returning DirCon characteristic %s with type %d", dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID.to128().toString().c_str(), dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).Type);
            returnMessage.AdditionalUUIDs.push_back(dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).UUID);
            returnMessage.AdditionalData.push_back(dirConServices.at(serviceIndex).Characteristics.at(characteristicIndex).Type);
          }
          serviceFound = true;
          break;
        }
      }
      if (!serviceFound)
      {
        log_e("Unknown service UUID %s for characteristic discovery requested, skipping further processing", dirConMessage[clientIndex].UUID.to128().toString().c_str());
        returnMessage.ResponseCode = DIRCON_RESPCODE_SERVICE_NOT_FOUND;
      }
      break;      
    case DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS:
      log_i("DirCon characteristic enable notification request for characteristic %s with additional data of %d bytes", dirConMessage[clientIndex].UUID.to128().toString().c_str(), dirConMessage[clientIndex].AdditionalData.size());
      returnMessage.ResponseCode = DIRCON_RESPCODE_SUCCESS_REQUEST;
      returnMessage.UUID = dirConMessage[clientIndex].UUID;
      break;
    default:
      log_e("Unknown identifier %d in DirCon message, skipping further processing", dirConMessage[clientIndex].Identifier);
      returnMessage.Identifier = DIRCON_MSGID_ERROR;
      break;
  }
  //std::vector<uint8_t> zwiftAsyncNotificationData = generateZwiftAsyncNotificationData(currentPower, currentCadence, 0LL, 0LL, 0LL);
  //std::vector<uint8_t> dirConPacket = generateDirConPacket(0x01, DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION, 0x00, 0x00, zwiftAsyncCharacteristicUUID, zwiftAsyncNotificationData);
  returnData = returnMessage.encode(dirConMessage[clientIndex].SequenceNumber);
  if (returnData.size() > 0) 
  {
    log_d("Sending data to DirCon client #%d with IP %s, hex value: %s", clientIndex, client->remoteIP().toString().c_str(), getHexString(returnData.data(), returnData.size()).c_str());
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
      dirConLastSequenceNumber[counter] = 0;
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
    dirConZwiftService.addCharacteristic(zwiftAsyncCharacteristicUUID, DIRCON_CHAR_PROP_FLAG_NOTIFY);
    dirConZwiftService.addCharacteristic(zwiftSyncRXCharacteristicUUID, DIRCON_CHAR_PROP_FLAG_WRITE);
    dirConZwiftService.addCharacteristic(zwiftSyncTXCharacteristicUUID, DIRCON_CHAR_PROP_FLAG_READ);
    dirConServices.push_back(dirConZwiftService);

    log_d("Iterating remote BLE services and characteristics...");
    std::vector<NimBLERemoteService *> *remoteServices = bLEClient->getServices(true);
    for (size_t serviceIndex = 0; serviceIndex < remoteServices->size(); serviceIndex++)
    {
      log_d("Found service %s, reading characteristics...", remoteServices->at(serviceIndex)->getUUID().to128().toString().c_str());
      DirConService dirConService(remoteServices->at(serviceIndex)->getUUID());
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