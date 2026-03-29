#include "image/image-codec.hpp"

#include "image/jpg-codec.hpp"
#include "image/png-codec.hpp"

namespace limb::image {
std::unique_ptr<Codec> CodecFactory::fromType(CodecType type, AllocationType at) {
  switch (type) {
  case CodecType::kPng:
    return std::make_unique<PngCodec>();
  case CodecType::kJpg: {
    Codec *codec = new (std::nothrow) JpgCodec;
    return std::unique_ptr<Codec>{codec};
  }
  default:
    return nullptr;
  }
}

std::unique_ptr<Codec> CodecFactory::fromData(std::span<const EncodedDataType> encoded, AllocationType at) {
  if (PngCodec::canDecode(encoded)) {
    return fromType(CodecType::kPng, at);
  } else if (JpgCodec::canDecode(encoded)) {
    return fromType(CodecType::kJpg, at);
  }

  return nullptr;
}
} // namespace limb::image