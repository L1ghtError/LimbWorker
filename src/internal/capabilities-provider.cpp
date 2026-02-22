#include "capabilities-provider.h"

#include <mutex>
#include <thread>

namespace limb {
class CapabilitiesProvider::impl {
public:
  impl() : m_appInfo(AppInfoTask{.totalCpuThreads = std::thread::hardware_concurrency()}) {}
  ~impl() {}

  liret addAvailableProcessor(std::string_view name, uint32_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &proc : m_appInfo.availableProcessors) {
      if (proc.index == index) {
        return liret::kAlreadyExists;
      }
    }

    m_appInfo.availableProcessors.push_back(AppInfoTask::AvailableProcessor{std::string(name), index});

    return liret::kOk;
  }

  liret removeAvailableProcessor(uint32_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto &vec = m_appInfo.availableProcessors;

    auto it = std::find_if(vec.begin(), vec.end(),
                           [index](const AppInfoTask::AvailableProcessor &proc) { return proc.index == index; });

    if (it == vec.end()) {
      return liret::kNotFound;
    }

    vec.erase(it);

    return liret::kOk;
  }

  void clear() { m_appInfo.availableProcessors.clear(); }

  AppInfoTask getAppInfo() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_appInfo;
  }

private:
  AppInfoTask m_appInfo;

  std::mutex m_mutex;
};

using Cp = CapabilitiesProvider;
liret Cp::addAvailableProcessor(std::string_view name, uint32_t index) {
  return pImpl->addAvailableProcessor(name, index);
}
liret Cp::removeAvailableProcessor(uint32_t index) { return pImpl->removeAvailableProcessor(index); }
void Cp::clear() { return pImpl->clear(); }
AppInfoTask Cp::getAppInfo() { return pImpl->getAppInfo(); }

CapabilitiesProvider::CapabilitiesProvider() : pImpl{std::make_unique<impl>()} {};
CapabilitiesProvider::CapabilitiesProvider(CapabilitiesProvider &&cp) : pImpl{std::move(cp.pImpl)} {}
CapabilitiesProvider::~CapabilitiesProvider() = default;

} // namespace limb
