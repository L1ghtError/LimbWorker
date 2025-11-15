#include "image-service/image-service.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "utils/stb-include.h"

namespace limb {

ImageService *imageService = nullptr;

ImageService::ImageService(std::unique_ptr<MediaRepository> mediaRepo) : m_mediaRepo(std::move(mediaRepo)) {};

ImageService::~ImageService() {}

liret ImageService::processImage(const MUpscaleImage &input, const ProgressCallback &&procb) {
  uint8_t *filedata;
  size_t size;

  liret ret = m_mediaRepo->getImageById((const char *)input.imageId, sizeof(input.imageId), &filedata, &size);
  if (ret != liret::kOk) {
    return ret;
  }

  int w, h, c;
  uint8_t *pixeldata = stbi_load_from_memory(filedata, size, &w, &h, &c, 0);
  delete filedata;
  ImageProcessor *imgProc;
  getProcessor((IMAGE_PROCESSOR_TYPES)input.modelId, &imgProc);
  if (ret != liret::kOk) {
    return ret;
  }

  ImageInfo inimage{.data = pixeldata, .w = w, .h = h, .c = c};
  ImageInfo outimage;
  imgProc->process_image(inimage, outimage, ProgressCallback(procb));
  delete pixeldata;
  int outSize = 0;
  uint8_t *outImage = stbi_write_png_to_mem((uint8_t *)outimage.data, 0, outimage.w, outimage.h, outimage.c, &outSize);

  ret = m_mediaRepo->updateImageById((const char *)input.imageId, sizeof(input.imageId), outImage, outSize);
  if (ret != liret::kOk) {
    return ret;
  }

  return liret::kOk;
}

liret ImageService::getProcessor(size_t index, ImageProcessor **processor) {
  if (index < 0 || index >= m_processors.size() || m_processors[index] == nullptr) {
    return liret::kInvalidInput;
  }
  *processor = m_processors[index];

  return liret::kOk;
}

liret ImageService::addProcessor(size_t index, ImageProcessor *processor) {
  if (index < 0 || processor == nullptr) {
    return liret::kInvalidInput;
  }
  if (m_processors.size() < index) {
    m_processors.resize(index + 1);
  }
  m_processors[index] = processor;

  return liret::kOk;
}

bool ImageService::removeProcessor(size_t index) {
  if (index < 0 || index >= m_processors.size() || m_processors[index] == nullptr) {
    return false;
  }
  m_processors[index] == nullptr;
  return true;
}
} // namespace limb
