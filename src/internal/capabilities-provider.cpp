#include "capabilities-provider.h"

#include <mutex>
#include <thread>

#include "simdjson.h"

namespace limb {
class CapabilitiesProvider::impl {
public:
  impl() : m_totalCpuThreads(std::thread::hardware_concurrency()) {}
  ~impl() {}

  liret addAvailableProcessor(std::string_view name, uint32_t index) {
    if (m_availableProcessors.size() < index + 1) {
      m_availableProcessors.resize(index + 1);
    } else if (!m_availableProcessors[index].empty()) {
      return liret::kAlreadyExists;
    }

    invalidate();

    m_availableProcessors[index] = name;
    return liret::kOk;
  }

  liret removeAvailableProcessor(uint32_t index) {
    if (m_availableProcessors.size() < index + 1) {
      return liret::kNotFound;
    }

    invalidate();

    m_availableProcessors.erase(m_availableProcessors.begin() + index);
    return liret::kOk;
  }

  void clear() { m_availableProcessors.clear(); }

  void invalidate() { isSync = false; }

  liret getCapabilitiesJSON(uint8_t *buf, size_t &size) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!isSync || sb.size() <= 0) {
      calcJSON();
    }

    const size_t required = sb.size();
    if (buf == nullptr) {
      size = required;
      return liret::kOk;
    } else if (size < required) {
      size = required;
      return liret::kBufferTooSmall;
    }

    std::memcpy(buf, sb.c_str().value(), required);
    size = required;
    return liret::kOk;
  }

  // Note: getCapabilitiesJSON wraps call with mutex
  void calcJSON() {
    isSync = true;
    sb.clear();
    sb.start_object();

    sb.append_key_value("cpuThds", m_totalCpuThreads);
    sb.append_comma();

    // array of image processors
    sb.escape_and_append_with_quotes("imgProc");
    sb.append_colon();
    sb.start_array();
    for (size_t i = 0; i < m_availableProcessors.size(); ++i) {
      sb.start_object();
      sb.append_key_value("i", i);
      sb.append_comma();
      sb.append_key_value("name", m_availableProcessors[i]);
      sb.append_comma();
      sb.end_object();
      if (i < m_availableProcessors.size() - 1) {
        sb.append_comma();
      }
    }
    sb.end_array();
    sb.end_object();
  }

  void lock() { m_mutex.lock(); }
  void unlock() { m_mutex.unlock(); }

private:
  simdjson::builder::string_builder sb;

  // TODO: resize in impl constructor
  std::vector<std::string> m_availableProcessors;
  uint32_t m_totalCpuThreads;

  std::mutex m_mutex;
  // check sync of actual data and its json representation
  bool isSync;
};

using Cp = CapabilitiesProvider;
liret Cp::addAvailableProcessor(std::string_view name, uint32_t index) { return pImpl->addAvailableProcessor(name, index); }
liret Cp::removeAvailableProcessor(uint32_t index) { return pImpl->removeAvailableProcessor(index); }
void Cp::clear() { return pImpl->clear(); }
void Cp::invalidate() { pImpl->invalidate(); }
liret Cp::getCapabilitiesJSON(uint8_t *buf, size_t &size) { return pImpl->getCapabilitiesJSON(buf, size); }

void Cp::lock() { return pImpl->lock(); }
void Cp::unlock() { return pImpl->unlock(); }

CapabilitiesProvider::CapabilitiesProvider() : pImpl{std::make_unique<impl>()} {};
CapabilitiesProvider::CapabilitiesProvider(CapabilitiesProvider &&cp) : pImpl{std::move(cp.pImpl)} {}
CapabilitiesProvider::~CapabilitiesProvider() = default;

} // namespace limb
