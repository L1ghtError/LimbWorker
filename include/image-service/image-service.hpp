#ifndef _IMAGE_SERVICE_HPP_
#define _IMAGE_SERVICE_HPP_

#include "processor-module.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "app-tasks/task-types.hpp"

#include "media-repository/media-repository.hpp"

#include "image/jpg-codec.hpp"
#include "image/png-codec.hpp"

#include "processor-initializer.hpp"
#include "processor-storage.hpp"

#include "utils/status.h"

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
    if (ret != liret::kOk) {
      return ret;
    }
    std::unique_ptr<uint8_t[]> inImageData(inImage);

    image::Container inPixel;
    auto codecFactory = image::CodecFactory::getInstance();

    std::span<image::EncodedDataType> imageSpan{inImage, size};
    auto codec = codecFactory->acquireFromData(imageSpan);
    ret = codec->decode(imageSpan, inPixel);
    if (ret != liret::kOk) {
      return ret;
    }

    auto container = getContainer(input.modelId);
    if (!container) {
      return liret::kAborted;
    }

    auto procDeleter = [&container](ImageProcessor *ptr) { container->reclaimProcessor(ptr); };
    std::unique_ptr<ImageProcessor, decltype(procDeleter)> processor(container->tryAcquireProcessor(), procDeleter);
    if (!processor) {
      return liret::kAborted;
    }

    ImageInfo inImageInfo{.data = inPixel.data.get(), .w = inPixel.w, .h = inPixel.h, .c = inPixel.c};
    ImageInfo outImageInfo;
    ret = processor->process_image(inImageInfo, outImageInfo, ProgressCallback(procb));
    if (ret != liret::kOk) {
      return ret;
    }

    image::Container outPixel{
        .data = image::ContainerData(outImageInfo.data, [](image::ContainerDataType *ptr) { delete[] ptr; }),
        .size = outImageInfo.size,
        .w = outImageInfo.w,
        .h = outImageInfo.h,
        .c = outImageInfo.c};

    const auto encodeCb = [this, &input](image::EncodeData data, size_t size) {
      return m_mediaRepo.updateImageById(input.imageId.c_str(), input.imageId.size(), data.get(), size);
    };

    using CodecType = limb::image::CodecType;
    if (outPixel.c == 4 && codec->type() == CodecType::kJpg) {
      auto pngCodec = codecFactory->acquireFromType(CodecType::kPng);
      return pngCodec->encode(outPixel, encodeCb);
    }

    return codec->encode(outPixel, encodeCb);
  }

  virtual size_t processorCount() { return m_processorProvider.processorCount(); }

  using reclaim = std::function<void(ProcessorContainer *)>;

  virtual std::unique_ptr<ProcessorContainer, reclaim> getContainer(size_t index) {
    const auto deleater = [this](ProcessorContainer *container) { m_processorProvider.reclaimContainer(container); };

    return std::unique_ptr<ProcessorContainer, reclaim>{m_processorProvider.acquireContainer(index), deleater};
  }

  virtual liret addContainer(size_t index, ProcessorContainer *container) {
    return m_processorProvider.addContainer(index, container);
  }

  virtual bool removeContainer(size_t index) { return m_processorProvider.removeContainer(index); }

  virtual void clear() { m_processorProvider.clear(); }

private:
  ProcessorInitializer<ProcessorStorage> m_processorProvider;

  Repo m_mediaRepo;
};
} // namespace limb

#endif // _IMAGE_SERVICE_HPP_