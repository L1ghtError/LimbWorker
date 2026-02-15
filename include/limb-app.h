#ifndef _LIMB_APP_H_
#define _LIMB_APP_H_

#include <memory>
#include <mutex> // lock_guard
#include <string>
#include <vector>

#include "capabilities-provider.h"
#include "image-service/image-service.hpp"
#include "processor-loader.h"
#include "utils/callbacks.h"

namespace limb {

class AppBase {
public:
  virtual ~AppBase() = default;
  virtual liret init() = 0;
  virtual void deinit() = 0;
  virtual liret processImage(const ImageTask &, const ProgressCallback && = [](float val) {}) = 0;
  virtual liret getCapabilitiesJSON(uint8_t *, size_t &) = 0;

  virtual size_t processorCount() const = 0;
  virtual std::string_view processorName(size_t index) = 0;
};

template <class MediaService, class Loader = ProcessorLoader, class CapProvider = CapabilitiesProvider>
class App : public AppBase {
public:
  template <typename M, typename L, typename C>
  App(M &&service, L &&loader, C &&capProvider)
      : m_mediaService(std::forward<M>(service)), m_processorLoader(std::forward<L>(loader)),
        m_capProvider(std::forward<C>(capProvider)){};

  ~App() override { deinit(); };

  liret init() override {
    liret err = liret::kOk;
    std::lock_guard<CapProvider> lock(m_capProvider);

    const size_t pc = m_processorLoader.processorCount();
    auto loaderDeleter = [this](ProcessorContainer *ptr) {
      ptr->deinit();
      this->m_processorLoader.destroyContainer(ptr);
    };

    for (size_t i = 0; i < pc; i++) {
      std::unique_ptr<ProcessorContainer, decltype(loaderDeleter)> container(m_processorLoader.allocateContainer(i),
                                                                             loaderDeleter);
      if (!container) {
        return liret::kInvalidInput;
      }

      // TODO: add lazy load, and add unload on idle
      err = container->init();
      if (err != liret::kOk) {
        return err;
      }

      err = m_mediaService.addContainer(i, container.get());
      if (err != liret::kOk) {
        return err;
      }

      m_capProvider.addAvailableProcessor(m_processorLoader.processorName(i), i);
      if (err != liret::kOk) {
        m_mediaService.removeContainer(i);
        return err;
      }
      container.release();
    }
    return err;
  }

  void deinit() override {
    std::lock_guard<CapProvider> lock(m_capProvider);
    const size_t pc = m_mediaService.processorCount();

    for (int i = 0; i < pc; i++) {
      ProcessorContainer *container;
      if (m_mediaService.getContainer(i, &container) == liret::kOk) {
        container->deinit();
        m_processorLoader.destroyContainer(container);
      }
    }
    m_capProvider.clear();
    m_mediaService.clear();
  }

  liret processImage(const ImageTask &input, const ProgressCallback &&procb) override {
    return m_mediaService.processImage(input, ProgressCallback(procb));
  }

  liret getCapabilitiesJSON(uint8_t *buf, size_t &size) override {
    return m_capProvider.getCapabilitiesJSON(buf, size);
  }

  size_t processorCount() const override { return m_processorLoader.processorCount(); }
  std::string_view processorName(size_t index) override { return m_processorLoader.processorName(index); }

  // private:
  MediaService m_mediaService;
  Loader m_processorLoader;
  CapProvider m_capProvider;
};
} // namespace limb

#endif // _LIMB_APP_H_