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
void handleWebServerFile(const String& fileName);
void handleWebServerStatus();

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
  Service* zwiftCustomService = new Service(NimBLEUUID(ZWIFT_CUSTOM_SERVICE_UUID), true, true);
  zwiftCustomService->addCharacteristic(new Characteristic(NimBLEUUID(ZWIFT_ASYNC_CHARACTERISTIC_UUID), NOTIFY));
  zwiftCustomService->addCharacteristic(new Characteristic(NimBLEUUID(ZWIFT_SYNCRX_CHARACTERISTIC_UUID), WRITE));
  zwiftCustomService->addCharacteristic(new Characteristic(NimBLEUUID(ZWIFT_SYNCTX_CHARACTERISTIC_UUID), INDICATE));
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
  webServer.on("/status", handleWebServerStatus);
  webServer.on("/favicon.ico", [] { handleWebServerFile("favicon.ico"); });
  webServer.on("/style.css", [] { handleWebServerFile("style.css"); });
  webServer.on("/", [] { handleWebServerFile("index.html"); });
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

extern const uint8_t favicon_ico_start[] asm("_binary_src_web_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_src_web_favicon_ico_end");

extern const uint8_t index_html_start[] asm("_binary_src_web_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_src_web_index_html_end");

extern const uint8_t style_css_start[] asm("_binary_src_web_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_src_web_style_css_end");

void handleWebServerFile(const String& fileName) {
  if (iotWebConf.handleCaptivePortal()) {
    return;
  }

  if (fileName.equals("index.html")) {
    webServer.send_P(200, "text/html", (char*)index_html_start, (index_html_end - index_html_start));
  }
  if (fileName.equals("style.css")) {
    webServer.send_P(200, "text/css", (char*)style_css_start, (style_css_end - style_css_start));
  }
  if (fileName.equals("favicon.ico")) {
    webServer.send_P(200, "image/x-icon", (char*)favicon_ico_start, (favicon_ico_end - favicon_ico_start));
  }
}

void handleWebServerStatus() {

  String json = "{";
  json += "\"devicename\": \"";
  json += Utils::getDeviceName().c_str();
  json += "\",";

  json += "\"version\": \"";
  json += VERSION;
  json += "\",";

  json += "\"hostname\": \"";
  json += Utils::getHostName().c_str();
  json += "\",";

  json += "\"ethernet_status\": \"";
  if (isEthernetConnected) {
    json += "Connected";
    json += ", IP: ";
    json += ETH.localIP().toString();
  } else {
    json += "Not connected";
  }
  json += "\",";

  json += "\"wifi_status\": \"";
  switch (iotWebConf.getState()) {
    case iotwebconf::NetworkState::ApMode:
      json += "Access-Point mode";
      break;
    case iotwebconf::NetworkState::Boot:
      json += "Booting";
      break;
    case iotwebconf::NetworkState::Connecting:
      json += "Connecting";
      break;
    case iotwebconf::NetworkState::NotConfigured:
      json += "Not configured";
      break;
    case iotwebconf::NetworkState::OffLine:
      json += "Offline";
      break;
    case iotwebconf::NetworkState::OnLine:
      json += "Online";
      json += ", SSID: ";
      json += WiFi.SSID();
      json += ", IP: ";
      json += WiFi.localIP().toString();
      break;
    default:
      json += "Unknown";
      break;
  }
  json += "\",";

  json += "\"ble_status\": \"";
  if (BTDeviceManager::isConnected()) {
    json += "Connected";
    json += ", ";
    json += BTDeviceManager::getConnecedDeviceName().c_str();
  } else {
    json += "Not connected";
  }
  json += "\",";

  String services_json = "\"ble_services\": [";
  for (Service* service : serviceManager.getServices()) {
    services_json += "\"";
    services_json += service->UUID.to128().toString().c_str();
    services_json += "\",";
    for(Characteristic* characteristic : service->getCharacteristics()) {
      services_json += "\"-- ";
      services_json += characteristic->UUID.to128().toString().c_str();
      services_json += "\",";
    }
  }
  if (services_json.endsWith(",")) {
    services_json.remove(services_json.length() - 1);
  }
  services_json += "],";

  json += services_json;

  json += "\"free_heap\": ";
  json += ESP.getFreeHeap();
  json += "}";
  
  webServer.send(200, "application/json", json);
}
  