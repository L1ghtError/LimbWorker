#ifndef _REALESRGAN_SERVICE_H_
#define _REALESRGAN_SERVICE_H_
#include "image-processor.h"

#include <gpu.h>
#include <layer.h>
#include <net.h>

namespace limb {
class RealesrganProcessor : public ImageProcessor {
public:
  RealesrganProcessor();
  RealesrganProcessor(ncnn::Net *net, bool tta_mode = false);
  ~RealesrganProcessor();

  virtual liret init() override;

  liret load() override;

  const char *name() override;

  liret process_image(
      const ImageInfo &inimage, ImageInfo &outimage,
      const ProgressCallback &&procb = [](float val) { printf("%.2f\n", val); }) const override;

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

  ncnn::Net *net;
  bool net_owner;
  bool tta_mode;
};
} // namespace limb

extern "C" limb::RealesrganProcessor *createProcessor();

extern "C" void destroyProcessor(limb::ImageProcessor *processor);

extern "C" const char *processorName();

#endif // _REALESRGAN_SERVICE_H_
