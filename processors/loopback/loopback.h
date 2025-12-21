#ifndef _LOOPBACK_PROCESSOR_H_
#define _LOOPBACK_PROCESSOR_H_

#include "image-processor.h"

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
class LIMB_API LoopbackProcessor : public ImageProcessor {
public:
  LoopbackProcessor();
  ~LoopbackProcessor();

  liret init() override;

  liret load() override;

  const char *name() override;

  liret process_image(
      const ImageInfo &inimage, ImageInfo &outimage,
      const ProgressCallback &&procb = [](float val) { printf("%.2f\n", val); }) const override;
};
} // namespace limb

extern "C" LIMB_API limb::LoopbackProcessor *createProcessor();

extern "C" LIMB_API void destroyProcessor(limb::ImageProcessor *processor);

extern "C" LIMB_API const char *processorName();

#endif // _LOOPBACK_PROCESSOR_H_
