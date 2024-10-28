#ifndef UUIDS_H
#define UUIDS_H

#include <NimBLEDevice.h>

NimBLEUUID zwiftCustomServiceUUID("00000001-19CA-4651-86E5-FA29DCDD09D1");
NimBLEUUID zwiftAsyncCharacteristicUUID("00000002-19CA-4651-86E5-FA29DCDD09D1");
NimBLEUUID zwiftSyncRXCharacteristicUUID("00000003-19CA-4651-86E5-FA29DCDD09D1");
NimBLEUUID zwiftSyncTXCharacteristicUUID("00000004-19CA-4651-86E5-FA29DCDD09D1");

NimBLEUUID cyclingSpeedAndCadenceServiceUUID("1816");
NimBLEUUID cyclingPowerServiceUUID("1818");

NimBLEUUID tacxCustomFECServiceUUID("6e40fec1-b5a3-f393-e0a9-e50e24dcca9e");

#endif 