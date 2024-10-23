#ifndef SERVICEMANAGERCALLBACKS_H
#define SERVICEMANAGERCALLBACKS_H

#include <Service.h>
#include <Characteristic.h>

class ServiceManagerCallbacks {

public:
    virtual ~ServiceManagerCallbacks() {}

    /**
     * @brief Called when a new service has been added
     * @param [in] service The service that was added.
     */
    virtual void onServiceAdded(Service* service) {};

    /**
     * @brief Called when a new service has been added
     * @param [in] service The service that was added.
     */
    virtual void onServiceRemoved(Service* service) {};

};

#endif