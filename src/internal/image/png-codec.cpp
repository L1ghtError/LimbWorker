#include "image/png-codec.hpp"

#include "image/image-utils.hpp"

#include "utils/status.h"
#include "utils/stb-wrap.h"

namespace {
void commonDeleter(void *ptr) {
  if (!ptr) {
    return;
  }
  stbi_image_free(ptr);
}
} // namespace

namespace limb::image {

liret PngCodec::decode(std::span<const EncodedDataType> encoded, Container &container) {
  if (!isPng(encoded)) {
    return liret::kInvalidInput;
  }
  int w, h, c;
  container.data.reset(stbi_load_from_memory(encoded.data(), encoded.size(), &w, &h, &c, 0));
  if (!container.data) {
    return liret::kAborted;
  }

  container.data.get_deleter() = commonDeleter;
  container.size = w * h * c;
  container.w = w;
  container.h = h;
  container.c = c;

  return liret::kOk;
};

liret PngCodec::encode(const Container &container, EncodeCb cb) {
  if (!container.data) {
    return liret::kInvalidInput;
  }
  int outSize;
  EncodeData data(lib_image_write_png_to_mem(container.data.get(), 0, container.w, container.h, container.c, &outSize),
                  commonDeleter);

  return cb(std::move(data), outSize);
};

} // namespace limb::image