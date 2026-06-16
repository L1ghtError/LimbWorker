#ifndef _PROCESSOR_PROVIDER_HPP_
#define _PROCESSOR_PROVIDER_HPP_

#include "processor-module.h"

#include "utils/status.h"

namespace limb {

class ProcessorProvider {
public:
  virtual liret addContainer(size_t index, ProcessorContainer *container) = 0;
  virtual bool removeContainer(size_t index) = 0;

  virtual ProcessorContainer *acquireContainer(std::size_t index) = 0;
  virtual void reclaimContainer(ProcessorContainer *container) = 0;

  virtual void clear() = 0;

  [[nodiscard]]
  virtual std::size_t processorCount() const noexcept = 0;
};

} // namespace limb
#endif // _PROCESSOR_PROVIDER_HPP_