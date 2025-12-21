#ifndef _LIMB_APP_H_
#define _LIMB_APP_H_

#include <memory>
#include <mutex> // lock_guard
#include <string>
#include <vector>

#include "utils/stb-include.h"

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
  virtual std::string processorName(size_t index) = 0;
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
    auto loaderDeleter = [this](ImageProcessor *ptr) { this->m_processorLoader.destroyProcessor(ptr); };

    for (size_t i = 0; i < pc; i++) {
      std::unique_ptr<ImageProcessor, decltype(loaderDeleter)> p(m_processorLoader.allocateProcessor(i), loaderDeleter);
      if (!p) {
        return liret::kInvalidInput;
      }

      // TODO: add lazy load, and add unload on idle
      err = p->init();
      if (err != liret::kOk) {
        return err;
      }
      err = p->load();
      if (err != liret::kOk) {
        return err;
      }

      err = m_mediaService.addProcessor(i, p.get());
      if (err != liret::kOk) {
        return err;
      }

      m_capProvider.addAvailableProcessor(m_processorLoader.processorName(i), i);
      if (err != liret::kOk) {
        m_mediaService.removeProcessor(i);
        return err;
      }
      p.release();
    }
    return err;
  }

  void deinit() override {
    std::lock_guard<CapProvider> lock(m_capProvider);

    for (int i = 0; i < m_mediaService.processorCount(); i++) {
      ImageProcessor *p;
      if (m_mediaService.getProcessor(i, &p) == liret::kOk) {
        m_processorLoader.destroyProcessor(p);
      }
      m_capProvider.clear();
      m_mediaService.clear();
    }
  }

  liret processImage(const ImageTask &input, const ProgressCallback &&procb) override {
    return m_mediaService.processImage(input, ProgressCallback(procb));
  }

  liret getCapabilitiesJSON(uint8_t *buf, size_t &size) override {
    return m_capProvider.getCapabilitiesJSON(buf, size);
  }

  size_t processorCount() const override { return m_processorLoader.processorCount(); }
  std::string processorName(size_t index) override { return m_processorLoader.processorName(index); }

  // private:
  MediaService m_mediaService;
  Loader m_processorLoader;
  CapProvider m_capProvider;
};
} // namespace limb

#endif // _LIMB_APP_H_