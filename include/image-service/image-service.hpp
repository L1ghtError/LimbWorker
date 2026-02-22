#ifndef _IMAGE_SERVICE_HPP_
#define _IMAGE_SERVICE_HPP_
#include "processor-module.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "app-tasks/task-types.hpp"
#include "media-repository/media-repository.hpp"

#include "utils/status.h"
#include "utils/stb-wrap.h"

namespace limb {

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

    ProcessorContainer *container;
    ret = getContainer(input.modelId, &container);
    if (ret != liret::kOk) {
      return ret;
    }

    auto procDeleter = [container](ImageProcessor *ptr) { container->reclaimProcessor(ptr); };
    std::unique_ptr<ImageProcessor, decltype(procDeleter)> processor(container->tryAcquireProcessor(), procDeleter);
    if (!processor) {
      return liret::kAborted;
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
        lib_image_write_png_to_mem(outPixel.get(), 0, outImageInfo.w, outImageInfo.h, outImageInfo.c, &outSize),
        stbiDeleter);

    return m_mediaRepo.updateImageById(input.imageId.c_str(), input.imageId.size(), outImage.get(), outSize);
  }

  virtual size_t processorCount() { return m_containers.size(); }

  virtual liret getContainer(size_t index, ProcessorContainer **container) {
    if (index >= m_containers.size() || m_containers[index] == nullptr) {
      return liret::kNotFound;
    }
    *container = m_containers[index];

    return liret::kOk;
  }

  virtual liret addContainer(size_t index, ProcessorContainer *container) {
    if (container == nullptr) {
      return liret::kInvalidInput;
    }
    if (m_containers.size() < index + 1) {
      m_containers.resize(index + 1);
    }
    m_containers[index] = container;

    return liret::kOk;
  }

  virtual bool removeContainer(size_t index) {
    if (index >= m_containers.size() || m_containers[index] == nullptr) {
      return false;
    }
    m_containers[index] = nullptr;
    return true;
  }

  virtual void clear() { m_containers.clear(); }

private:
  std::vector<ProcessorContainer *> m_containers;
  Repo m_mediaRepo;
};
} // namespace limb

#endif // _IMAGE_SERVICE_HPP_