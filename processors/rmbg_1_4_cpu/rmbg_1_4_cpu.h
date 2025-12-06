#ifndef _RMBG_1_4_PROCESSOR_H_
#define _RMBG_1_4_PROCESSOR_H_
#include "image-processor.h"

#ifdef _WIN32
#ifdef REALESRGAN_EXPORTS
#define LIMB_API __declspec(dllexport)
#else
#define LIMB_API __declspec(dllimport)
#endif
#else
#define LIMB_API
#endif

namespace limb {
class LIMB_API RMBGProcessor : public ImageProcessor {
public:
  RMBGProcessor();
  RMBGProcessor();
  ~RMBGProcessor();

  virtual liret init() override;

  liret load() override;

  const char *name() override;

  liret process_image(
      const ImageInfo &inimage, ImageInfo &outimage,
      const ProgressCallback &&procb = [](float val) { printf("%.2f\n", val); }) const override;
};
} // namespace limb

extern "C" LIMB_API limb::RMBGProcessor *createProcessor();

extern "C" LIMB_API void destroyProcessor(limb::ImageProcessor *processor);

extern "C" LIMB_API const char *processorName();

#endif // _RMBG_1_4_PROCESSOR_H_
