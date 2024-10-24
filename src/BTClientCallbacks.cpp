#include <BTClientCallbacks.h>
#include <BTDeviceManager.h>

void BTClientCallbacks::onConnect(NimBLEClient *pClient) {
  log_d("Connected to %s with RSSI %d", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());
  BTDeviceManager::connected = true;
};

void BTClientCallbacks::onDisconnect(NimBLEClient *pClient, int reason) {
  log_d("Disconnected from %s, reason %d", pClient->getPeerAddress().toString().c_str(), reason);
  BTDeviceManager::connected = false;
};
