#ifndef _IMAGE_SERVICE_HPP_
#define _IMAGE_SERVICE_HPP_
#include "image-processor.h"

#include <cstdint>
#include <vector>

#include "utils/status.h"

typedef enum {
  IP_IMAGE_NO = 0,
  IP_IMAGE_REALESRGAN, ///< ncnn realesrgan
} IMAGE_PROCESSOR_TYPES;

namespace limb {
struct MUpscaleImage {
  uint32_t modelId;
  uint8_t imageId[24];

  liret berawtoh(const char *raw) {
    *this = *(MUpscaleImage *)(raw);
    modelId = be32toh(modelId);
    return liret::kOk;
  }
};

class ImageService;
struct ImageServiceOptions {
private:
public:
  const char *paramfullpath;
  const char *modelfullpath;
  int vulkan_device_index;
  int prepadding;
  int tilesize;
  int scale;
  bool tta_mode;
  friend ImageService;
};

class ImageService {
public:
  ~ImageService();

  liret processImage(const MUpscaleImage &input, const ProgressCallback &&procb = [](float val) {});

  liret getProcessor(size_t index, ImageProcessor **processor);

  liret addProcessor(size_t index, ImageProcessor *proc);

  bool removeProcessor(size_t index);

private:
  std::vector<ImageProcessor *> m_processors;
};
extern ImageService *imageService;
} // namespace limb

#endif // _IMAGE_SERVICE_HPP_