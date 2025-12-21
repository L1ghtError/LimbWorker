#ifndef _CAPABILITIES_PROVIDER_H_
#define _CAPABILITIES_PROVIDER_H_

#include <cstdint>
#include <memory>
#include <string>


#include "utils/status.h"

namespace limb {

class CapabilitiesProvider {
public:
  CapabilitiesProvider();
  ~CapabilitiesProvider();
  CapabilitiesProvider(CapabilitiesProvider && cp);

  // Add a processor so the capabilities provider will report that we are capable of using it
  liret addAvailableProcessor(std::string name, uint32_t index);

  liret removeAvailableProcessor(uint32_t index);

  void clear();

  // To prevent json becoming stale between calls
  // before data modification should ALWAYS lock it
  void lock();
  void unlock();

  // there is chache that prevents json recalculation, it can be invalidated
  void invalidate();

  // return capabilities in json format with two-call pattern
  liret getCapabilitiesJSON(uint8_t *buf, size_t &size);

private:
  class impl;
  std::unique_ptr<impl> pImpl;
};
} // namespace limb
#endif // _CAPABILITIES_PROVIDER_H_