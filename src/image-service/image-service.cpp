#include "image-service/image-service.hpp"

#include "mongo-client/mongo-client.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "utils/stb-include.h"

namespace limb {

ImageService *imageService = nullptr;

ImageService::~ImageService() {
  // TODO: investigate nessesity of manually manage .net lifetime
  // for (int i = 0; i < m_services.size(); i++) {
  //   delete m_services[i].net;
  // }
}
liret ImageService::processImage(const MUpscaleImage &input, const ProgressCallback &&procb) {
  bson_oid_t imageId;
  // input.imageId Need to be uint8_t[24]
  bson_oid_init_from_string(&imageId, (const char *)input.imageId);
  uint8_t *filedata;
  ssize_t size;

  liret ret = limb::mongoService->getImageById(&imageId, &filedata, &size);
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
  uint8_t *outImage =
      stbi_write_png_to_mem((uint8_t *)outimage.data, 0, outimage.w, outimage.h, outimage.c, &outSize);

  ret = limb::mongoService->updateImageById(&imageId, outImage, outSize);
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
