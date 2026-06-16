#ifndef _PROCESSOR_INITIALIZER_HPP_
#define _PROCESSOR_INITIALIZER_HPP_

#include "processor-provider.hpp"

#include <concepts>
#include <set>

namespace limb {

template <class Upstream>
  requires std::derived_from<std::remove_cvref_t<Upstream>, ProcessorProvider>
class ProcessorInitializer : public ProcessorProvider {
public:
  template <typename T> ProcessorInitializer(T &&upstream) : m_upstream(std::forward<T>(upstream)){};

  ProcessorInitializer() = default;

  liret addContainer(size_t index, ProcessorContainer *container) override {
    return m_upstream.addContainer(index, container);
  };

  bool removeContainer(size_t index) override {

    deinitContainer(index);

    return m_upstream.removeContainer(index);
  }

  ProcessorContainer *acquireContainer(std::size_t index) override {
    auto *container = m_upstream.acquireContainer(index);

    if (!m_initializedMap.contains(index)) {
      m_initializedMap.insert(index);
      container->init();
    }

    return container;
  }

  void reclaimContainer(ProcessorContainer *container) override { m_upstream.reclaimContainer(container); }

  void clear() override {
    for (const auto index : m_initializedMap) {
      deinitContainer(index);
    }
    m_upstream.clear();
  }

  [[nodiscard]]
  std::size_t processorCount() const noexcept override {
    return m_upstream.processorCount();
  };

private:
  void deinitContainer(size_t index) {
    if (!m_initializedMap.contains(index)) {
      return;
    }

    auto *container = m_upstream.acquireContainer(index);
    if (!container) {
      return;
    }

    container->deinit();
    m_upstream.reclaimContainer(container);
    m_initializedMap.erase(index);
  }

  std::set<size_t> m_initializedMap;

  Upstream m_upstream;
};

} // namespace limb
#endif // _PROCESSOR_INITIALIZER_HPP_