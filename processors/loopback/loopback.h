#ifndef _LOOPBACK_PROCESSOR_H_
#define _LOOPBACK_PROCESSOR_H_

#include "processor-module.h"

// This is example/no-op processor that just copies input image to output image
// it may be used for learning purposes or as a template for new processors

#ifdef _WIN32
#ifdef LOOPBACK_EXPORTS
#define LIMB_API __declspec(dllexport)
#else
#define LIMB_API __declspec(dllimport)
#endif
#else
#define LIMB_API
#endif

namespace limb {

constexpr auto g_processorName = "Loopback";

class LoopbackProcessor : public ImageProcessor {
public:
  LoopbackProcessor();
  ~LoopbackProcessor() override;

  std::string_view name() const override { return g_processorName; };

  liret process_image(const ImageInfo &inimage, ImageInfo &outimage,
                      const ProgressCallback &procb = defaultProgressCallback) const override;
};

class LoopbackContainer : public ProcessorContainer {
public:
  LoopbackContainer();
  ~LoopbackContainer() override;

  liret init() override;
  liret deinit() override;

  ImageProcessor *tryAcquireProcessor() override;
  void reclaimProcessor(ImageProcessor *proc) override;

  std::string_view name() const override { return g_processorName; };
};

class LIMB_API LoopbackModule : public ProcessorModule {
public:
  ProcessorContainer *allocateContainer() override { return new LoopbackContainer; }

  void deallocateContainer(ProcessorContainer *proc) override { delete proc; }

  std::string_view name() const override { return g_processorName; }
};
} // namespace limb

extern "C" LIMB_API limb::LoopbackModule *GetProcessorModule();

#endif // _LOOPBACK_PROCESSOR_H_
