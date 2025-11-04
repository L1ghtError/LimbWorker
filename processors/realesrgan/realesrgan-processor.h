#ifndef _REALESRGAN_SERVICE_H_
#define _REALESRGAN_SERVICE_H_
#include "image-processor.h"

#include <gpu.h>
#include <layer.h>
#include <net.h>

namespace limb {
class RealesrganProcessor : public ImageProcessor {
public:
  RealesrganProcessor(ncnn::Net *net, bool tta_mode = false);
  ~RealesrganProcessor();
  liret load() override;

  const char* name() override;

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
  bool tta_mode;
};
} // namespace limb
#endif // _REALESRGAN_SERVICE_H_
