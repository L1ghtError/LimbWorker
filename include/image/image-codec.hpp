#ifndef _IMAGE_CODEC_HPP_
#define _IMAGE_CODEC_HPP_

#include "image-types.h"

#include <memory>
#include <span>

namespace limb::image {

class Codec {
public:
  virtual ~Codec() = default;

  virtual liret decode(std::span<const EncodedDataType> encoded, Container &container) = 0;

  virtual liret encode(const Container &container, EncodeCb cb) = 0;
};

enum class AllocationType { Initial = 0, Lazy = 1 };

class CodecFactory {
public:
  [[nodiscard]]
  static std::unique_ptr<Codec> fromType(CodecType type, AllocationType at = AllocationType::Lazy);
  [[nodiscard]]
  static std::unique_ptr<Codec> fromData(std::span<const EncodedDataType> encoded,
                                         AllocationType at = AllocationType::Lazy);
};

} // namespace limb::image

#endif // _IMAGE_CODEC_HPP_