#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <Service.h>

class ServiceManager {
  public:
    ServiceManager();
    std::vector<Service> getServices();
  private:
    std::vector<Service> Services;
};

#endif
