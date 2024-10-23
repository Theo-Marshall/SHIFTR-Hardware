#include <ServiceManager.h>
#include <Service.h>

ServiceManager::ServiceManager() {}

std::vector<Service> ServiceManager::getServices()
{
  return this->Services;
}
