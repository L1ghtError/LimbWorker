#include "image/image-codec.hpp"

#include "image/png-codec.hpp"

#include "image/image-utils.hpp"

namespace limb::image {
std::unique_ptr<Codec> CodecFactory::fromType(CodecType type) {
  switch (type) {
  case CodecType::kPng: {
    const auto ptr = new (std::nothrow) PngCodec();
    return std::unique_ptr<Codec>{ptr};
  }
  case CodecType::kJpg:
    return nullptr; // new (std::nothrow) JpgCodec();
  default:
    return nullptr;
  }
}

std::unique_ptr<Codec> CodecFactory::fromData(std::span<const EncodedDataType> encoded) {
  if (isPng(encoded)) {
    const auto ptr = new (std::nothrow) PngCodec();
    return std::unique_ptr<Codec>{ptr};
  }
  return nullptr;
}
} // namespace limb::image