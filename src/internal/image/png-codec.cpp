#include "image/png-codec.hpp"

#include "utils/status.h"
#include "utils/stb-wrap.h"

#include <array>

namespace {

constexpr std::array<unsigned char, 8> pngSignature = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

void commonDeleter(void *ptr) {
  if (!ptr) {
    return;
  }
  stbi_image_free(ptr);
}
} // namespace

namespace limb::image {

bool PngCodec::canDecode(std::span<const EncodedDataType> encoded) {
  if (encoded.size() < pngSignature.size())
    return false;

  for (size_t i = 0; i < pngSignature.size(); ++i) {
    if (encoded[i] != pngSignature[i])
      return false;
  }

  return true;
}

liret PngCodec::decode(std::span<const EncodedDataType> encoded, Container &container) {
  if (!canDecode(encoded)) {
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