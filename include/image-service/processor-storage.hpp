#ifndef _PROCESSOR_STORAGE_HPP_
#define _PROCESSOR_STORAGE_HPP_

#include "processor-provider.hpp"

#include "utils/status.h"

#include <algorithm>
#include <map>

namespace limb {

class ProcessorStorage : public ProcessorProvider {
public:
  liret addContainer(size_t index, ProcessorContainer *container) override {
    if (m_containers.find(index) != m_containers.end()) {
      return liret::kAlreadyExists;
    }

    m_containers[index] = container;
    return liret::kOk;
  };

  bool removeContainer(size_t index) override { return m_containers.erase(index) > 0; }

  ProcessorContainer *acquireContainer(std::size_t index) override { return m_containers.at(index); };
  void reclaimContainer(ProcessorContainer *container) override {};

  void clear() override { m_containers.clear(); };

  [[nodiscard]]
  std::size_t processorCount() const noexcept override {
    return m_containers.size();
  };

private:
  std::map<size_t, ProcessorContainer *> m_containers;
};

} // namespace limb
#endif // _PROCESSOR_STORAGE_HPP_