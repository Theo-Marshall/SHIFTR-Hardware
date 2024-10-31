#ifndef SERVICEMANAGERCALLBACKS_H
#define SERVICEMANAGERCALLBACKS_H

#include <Service.h>

class ServiceManagerCallbacks {
 public:
  virtual ~ServiceManagerCallbacks() {}
  virtual void onServiceAdded(Service* service) {};
  virtual void onCharacteristicSubscriptionChanged(Characteristic* characteristic, bool removed) {};
  // virtual void onServiceRemoved(Service* service) {};
};

#endif