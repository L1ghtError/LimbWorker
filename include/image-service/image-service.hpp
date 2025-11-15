#ifndef _IMAGE_SERVICE_HPP_
#define _IMAGE_SERVICE_HPP_
#include "image-processor.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "utils/endian.h"
#include "utils/status.h"

#include "media-repository/media-repository.hpp"

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

class ImageService {
public:
  ImageService(std::unique_ptr<MediaRepository> mediaRepo);

  ~ImageService();

  liret processImage(const MUpscaleImage &input, const ProgressCallback &&procb = [](float val) {});

  liret getProcessor(size_t index, ImageProcessor **processor);

  liret addProcessor(size_t index, ImageProcessor *proc);

  bool removeProcessor(size_t index);

private:
  std::vector<ImageProcessor *> m_processors;
  std::unique_ptr<MediaRepository> m_mediaRepo;
};

extern ImageService *imageService;
} // namespace limb

#endif // _IMAGE_SERVICE_HPP_