#ifndef _REALESRGAN_PROCESSOR_H_
#define _REALESRGAN_PROCESSOR_H_
#include "processor-module.h"

#ifdef _WIN32
#ifdef REALESRGAN_EXPORTS
#define LIMB_API __declspec(dllexport)
#else
#define LIMB_API __declspec(dllimport)
#endif
#else
#define LIMB_API
#endif

#include <gpu.h>
#include <layer.h>
#include <net.h>

namespace limb {

constexpr auto g_processorName = "Real-ESRGAN";

class LIMB_API RealesrganProcessor : public ImageProcessor {
public:
  RealesrganProcessor(const ncnn::Net *net, bool tta_mode = false);
  ~RealesrganProcessor() override;

  std::string_view name() const override { return g_processorName; };

  liret process_image(const ImageInfo &inimage, ImageInfo &outimage,
                      const ProgressCallback &procb = defaultProgressCallback) const override;

public:
  int scale;
  int tilesize;
  int prepadding;

private:
  ncnn::Pipeline *realesrgan_preproc;
  ncnn::Pipeline *realesrgan_postproc;
  ncnn::Layer *bicubic_2x;
  ncnn::Layer *bicubic_3x;
  ncnn::Layer *bicubic_4x;

  const ncnn::Net *net;
  bool tta_mode;
};

class RealesrganContainer : public ProcessorContainer {
public:
  RealesrganContainer();
  ~RealesrganContainer() override;

  liret init() override;
  liret deinit() override;

  ImageProcessor *tryAcquireProcessor() override;
  void reclaimProcessor(ImageProcessor *proc) override;

  std::string_view name() const override { return g_processorName; };

private:
  ncnn::Net *net;
};

class LIMB_API RealesrganModule : public ProcessorModule {
public:
  ProcessorContainer *allocateContainer() override { return new RealesrganContainer; }

  void deallocateContainer(ProcessorContainer *proc) override { delete proc; }

  std::string_view name() const override { return g_processorName; }
};

} // namespace limb

extern "C" LIMB_API limb::RealesrganModule *GetProcessorModule();

#endif // _REALESRGAN_PROCESSOR_H_
