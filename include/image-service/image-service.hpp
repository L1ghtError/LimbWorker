#ifndef _IMAGE_SERVICE_HPP_
#define _IMAGE_SERVICE_HPP_
#include "image-processor.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "utils/status.h"

#include "media-repository/media-repository.hpp"

#include "utils/stb-include.h"

namespace limb {
struct ImageTask {
  uint32_t modelId;
  std::string imageId;
};

template <class Repo>
  requires std::derived_from<std::remove_cvref_t<Repo>, MediaRepository>
class ImageService {
public:
  template <typename T> ImageService(T &&repo) : m_mediaRepo(std::forward<T>(repo)){};

  ImageService(const ImageService &) = delete;
  ImageService &operator=(const ImageService &) = delete;

  ImageService(ImageService &&is) = default;
  ImageService &operator=(ImageService &&) = default;

  virtual ~ImageService() = default;

  virtual liret processImage(const ImageTask &input, const ProgressCallback &&procb = [](float val) {}) {
    uint8_t *inImage;
    size_t size;

    liret ret = m_mediaRepo.getImageById(input.imageId.c_str(), input.imageId.size(), &inImage, &size);
    std::unique_ptr<uint8_t[]> inImageData(inImage);
    if (ret != liret::kOk) {
      return ret;
    }

    auto stbiDeleter = [](uint8_t *ptr) { stbi_image_free(ptr); };

    int w, h, c;
    std::unique_ptr<uint8_t[], decltype(stbiDeleter)> inPixel(
        stbi_load_from_memory(inImageData.get(), size, &w, &h, &c, 0), stbiDeleter);

    ImageProcessor *processor;
    ret = getProcessor(input.modelId, &processor);
    if (ret != liret::kOk) {
      return ret;
    }

    ImageInfo inImageInfo{.data = inPixel.get(), .w = w, .h = h, .c = c};
    ImageInfo outImageInfo;
    ret = processor->process_image(inImageInfo, outImageInfo, ProgressCallback(procb));
    std::unique_ptr<uint8_t[]> outPixel(outImageInfo.data);
    if (ret != liret::kOk) {
      return ret;
    }

    int outSize = 0;
    std::unique_ptr<uint8_t[], decltype(stbiDeleter)> outImage(
        stbi_write_png_to_mem(outPixel.get(), 0, outImageInfo.w, outImageInfo.h, outImageInfo.c, &outSize),
        stbiDeleter);

    return m_mediaRepo.updateImageById(input.imageId.c_str(), input.imageId.size(), outImage.get(), outSize);
  }

  virtual size_t processorCount() { return m_processors.size(); }

  virtual liret getProcessor(size_t index, ImageProcessor **processor) {
    if (index >= m_processors.size() || m_processors[index] == nullptr) {
      return liret::kNotFound;
    }
    *processor = m_processors[index];

    return liret::kOk;
  }

  virtual liret addProcessor(size_t index, ImageProcessor *processor) {
    if (processor == nullptr) {
      return liret::kInvalidInput;
    }
    if (m_processors.size() < index + 1) {
      m_processors.resize(index + 1);
    }
    m_processors[index] = processor;

    return liret::kOk;
  }

  virtual bool removeProcessor(size_t index) {
    if (index >= m_processors.size() || m_processors[index] == nullptr) {
      return false;
    }
    m_processors[index] = nullptr;
    return true;
  }

  virtual void clear() { m_processors.clear(); }

private:
  std::vector<ImageProcessor *> m_processors;
  Repo m_mediaRepo;
};
} // namespace limb

#endif // _IMAGE_SERVICE_HPP_