#ifndef BTCLIENTCALLBACKS_H
#define BTCLIENTCALLBACKS_H

#include <NimBLEDevice.h>

class BTClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient);
  void onDisconnect(NimBLEClient* pClient, int reason);
  bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params);
  void onPassKeyEntry(NimBLEConnInfo& connInfo);
  void onAuthenticationComplete(NimBLEConnInfo& connInfo);
  void onConfirmPIN(NimBLEConnInfo& connInfo, uint32_t pin);
  void onIdentity(NimBLEConnInfo& connInfo);
};

#endif