#include <BTClientCallbacks.h>
#include <BTDeviceManager.h>

void BTClientCallbacks::onConnect(NimBLEClient *pClient) {
  BTDeviceManager::connected = true;
};

void BTClientCallbacks::onDisconnect(NimBLEClient *pClient, int reason) {
  BTDeviceManager::connected = false;
};
