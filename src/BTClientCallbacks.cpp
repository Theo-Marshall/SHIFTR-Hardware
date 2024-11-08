#include <BTClientCallbacks.h>
#include <BTDeviceManager.h>

void BTClientCallbacks::onConnect(NimBLEClient *pClient) {
  log_d("Connected to %s with RSSI %d", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());
  BTDeviceManager::connected = true;
};

void BTClientCallbacks::onDisconnect(NimBLEClient *pClient, int reason) {
  log_d("Disconnected from %s, reason %d", pClient->getPeerAddress().toString().c_str(), reason);
  BTDeviceManager::connected = false;
  BTDeviceManager::connectedDeviceName = "";
  BTDeviceManager::startScan();

};

bool BTClientCallbacks::onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
  log_d("Connection parameter update request from %s", pClient->getPeerAddress().toString().c_str());
  return true;
}

