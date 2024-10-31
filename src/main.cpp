#include <Arduino.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <BTDeviceManager.h>
#include <Config.h>
#include <DirConManager.h>
#include <ESPmDNS.h>
#include <ETH.h>
#include <IotWebConf.h>
#include <IotWebConfESP32HTTPUpdateServer.h>
#include <NimBLEDevice.h>
#include <UUIDs.h>
#include <Utils.h>
#include <Version.h>
#include <WiFi.h>

void networkEvent(WiFiEvent_t event);
void handleWebServerRoot();

bool isMDNSStarted = false;
bool isBLEConnected = false;
bool isEthernetConnected = false;
bool isWiFiConnected = false;
bool isOTAInProgress = false;

DNSServer dnsServer;
WebServer webServer(WEB_SERVER_PORT);
HTTPUpdateServer updateServer;
IotWebConf iotWebConf(Utils::getDeviceName().c_str(), &dnsServer, &webServer, Utils::getHostName().c_str(), WIFI_CONFIG_VERSION);
ServiceManager serviceManager;

void setup() {
  log_i(DEVICE_NAME_PREFIX " " VERSION " starting...");
  log_i("Device name: %s, host name: %s", Utils::getDeviceName().c_str(), Utils::getHostName().c_str());
  // initialize service manager
  Service zwiftCustomService(zwiftCustomServiceUUID, true, true);
  zwiftCustomService.addCharacteristic(Characteristic(zwiftAsyncCharacteristicUUID, NOTIFY));
  zwiftCustomService.addCharacteristic(Characteristic(zwiftSyncRXCharacteristicUUID, WRITE));
  zwiftCustomService.addCharacteristic(Characteristic(zwiftSyncTXCharacteristicUUID, INDICATE));
  serviceManager.addService(zwiftCustomService);
  log_i("Service manager initialized");

  // initialize bluetooth device manager
  BTDeviceManager::setLocalDeviceName(Utils::getDeviceName());
  BTDeviceManager::setServiceManager(&serviceManager);
  BTDeviceManager::setRemoteDeviceNameFilter(DEFAULT_BLE_REMOTE_DEVICE_NAME_PREFIX);
  if (!BTDeviceManager::start()) {
    log_e("Startup failed: Unable to start bluetooth device manager");
    ESP.restart();
  }
  log_i("Bluetooth device manager initialized");

  // initialize OTA
  ArduinoOTA.onStart([]() {
    log_i("OTA started");
    isOTAInProgress = true;
  });
  ArduinoOTA.onEnd([]() {
    log_i("OTA finished, rebooting...");
    delay(1000);
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    log_i("OTA progress: %u%%", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    log_e("OTA error: %u, rebooting...", error);
    delay(1000);
    ESP.restart();
  });
  ArduinoOTA.setHostname(Utils::getHostName().c_str());
  ArduinoOTA.setMdnsEnabled(false);
  ArduinoOTA.setPassword(STR(OTA_PASSWORD));
  log_i("OTA initialized");

  // initialize network events
  WiFi.onEvent(networkEvent);
  log_i("Network events initialized");

  // initialize ethernet interface
  ETH.begin();
  log_i("Ethernet interface initialized");

  // initialize wifi manager and web server
  iotWebConf.setStatusPin(WIFI_STATUS_PIN);
  iotWebConf.setConfigPin(WIFI_CONFIG_PIN);
  iotWebConf.setupUpdateServer(
      [](const char* updatePath) { updateServer.setup(&webServer, updatePath); },
      [](const char* userName, char* password) { updateServer.updateCredentials(STR(OTA_USERNAME), STR(OTA_PASSWORD)); });
  iotWebConf.init();
  webServer.on("/", handleWebServerRoot);
  webServer.on("/config", [] { iotWebConf.handleConfig(); });
  webServer.onNotFound([]() { iotWebConf.handleNotFound(); });
  log_i("WiFi manager and web server initialized");

  // initialize MDNS
  if (!MDNS.begin(Utils::getHostName().c_str())) {
    log_e("Startup failed: Unable to start MDNS");
    ESP.restart();
  }
  MDNS.setInstanceName(Utils::getDeviceName().c_str());
  isMDNSStarted = true;
  log_i("MDNS initialized");

  // initialize DirCon manager
  DirConManager::setServiceManager(&serviceManager);
  if (!DirConManager::start()) {
    log_e("Startup failed: Unable to start DirCon manager");
    ESP.restart();
  }
  log_i("DirCon Manager initialized");

  log_i("Startup finished");
}

void loop() {
  if (!isOTAInProgress) {
    BTDeviceManager::update();
    DirConManager::update();
    iotWebConf.doLoop();
  } else {
    ArduinoOTA.handle();
  }
}

void networkEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      log_d("Ethernet started");
      ETH.setHostname(Utils::getHostName().c_str());
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      log_i("Ethernet connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      log_i("Ethernet DHCP successful with IP %u.%u.%u.%u", ETH.localIP()[0], ETH.localIP()[1], ETH.localIP()[2], ETH.localIP()[3]);
      isEthernetConnected = true;
      ArduinoOTA.end();
      ArduinoOTA.begin();
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      log_i("Ethernet disconnected");
      isEthernetConnected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      log_d("Ethernet stopped");
      isEthernetConnected = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      log_i("WiFi DHCP successful with IP %u.%u.%u.%u", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      isWiFiConnected = true;
      ArduinoOTA.end();
      ArduinoOTA.begin();
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      log_i("WiFi disconnected");
      isWiFiConnected = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      log_d("WiFi stopped");
      isWiFiConnected = false;
      break;
    default:
      break;
  }
}

void handleWebServerRoot() {
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal()) {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>";
  s += Utils::getDeviceName().c_str();
  s += "</title></head><body>";
  s += "<h1>";
  s += Utils::getDeviceName().c_str();
  s += " " VERSION;
  s += "</h1>";
  s += "<p>Ethernet: ";
  if (isEthernetConnected) {
    s += "Connected";
    s += ", IP: ";
    s += ETH.localIP().toString();
  } else {
    s += "Not connected";
  }
  s += "</p>";
  s += "<p>WiFi: ";
  switch (iotWebConf.getState()) {
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
  if (isBLEConnected) {
    s += "Connected";
    s += ", device: ";
    s += BTDeviceManager::getConnecedDeviceName().c_str();
  } else {
    s += "Not connected";
  }
  s += "</p>";
  s += "Go to <a href='config'>configuration</a> to change settings.";
  s += "</body></html>\n";

  webServer.send(200, "text/html", s);
}