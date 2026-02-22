#ifndef _CAPABILITIES_PROVIDER_H_
#define _CAPABILITIES_PROVIDER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "app-tasks/task-types.hpp"

#include "utils/status.h"

namespace limb {

class CapabilitiesProvider {
public:
  CapabilitiesProvider();
  ~CapabilitiesProvider();
  CapabilitiesProvider(CapabilitiesProvider &&cp);

  // Add a processor so the capabilities provider will report that we are capable of using it
  liret addAvailableProcessor(std::string_view name, uint32_t index);

  liret removeAvailableProcessor(uint32_t index);

  void clear();

  AppInfoTask getAppInfo();

private:
  class impl;
  std::unique_ptr<impl> pImpl;
};
} // namespace limb
#endif // _CAPABILITIES_PROVIDER_H_