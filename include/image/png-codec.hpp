#ifndef _PNG_CODEC_HPP_
#define _PNG_CODEC_HPP_

#include "image-codec.hpp"

namespace limb::image {

class PngCodec : public Codec {
public:
  PngCodec() = default;
  ~PngCodec() override = default;

  static bool canDecode(std::span<const EncodedDataType> encoded);

  liret decode(std::span<const EncodedDataType> encoded, Container &container) override;

  liret encode(const Container &container, EncodeCb cb) override;

  CodecType type() const override;
};

} // namespace limb::image

#endif // _PNG_CODEC_HPP_