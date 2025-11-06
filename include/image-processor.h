#ifndef _IMAGE_PROCESSOR_H_
#define _IMAGE_PROCESSOR_H_

#include "utils/callbacks.h"
#include "utils/image-info.h"
#include "utils/status.h"

#include <cstdio>
#include <string>

namespace limb {
class ImageProcessor {
public:
  virtual ~ImageProcessor() = default;

  virtual liret init() = 0;

  virtual liret load() = 0;

  virtual const char *name() = 0;
  // TODO: Make Lambda Embty (caller passes desired logger)
  virtual liret process_image(
      const ImageInfo &inimage, ImageInfo &outimage,
      const ProgressCallback &&procb = [](float val) { printf("%.2f\n", val); }) const = 0;
};
} // namespace limb
#endif // _IMAGE_PROCESSOR_H_