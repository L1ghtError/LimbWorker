#ifndef _JPG_CODEC_HPP_
#define _JPG_CODEC_HPP_

#include "image-codec.hpp"

namespace limb::image {

class JpgCodec : public Codec {
public:
  JpgCodec();
  ~JpgCodec() override;

  static bool canDecode(std::span<const EncodedDataType> encoded);

  liret decode(std::span<const EncodedDataType> encoded, Container &container) override;

  liret encode(const Container &container, EncodeCb cb) override;

  CodecType type() const override;

private:
  static void _tjDeleter(void *ptr);

private:
  std::unique_ptr<void, decltype(&_tjDeleter)> m_jpegCompressor;
  std::unique_ptr<void, decltype(&_tjDeleter)> m_jpegDecompressor;
};

} // namespace limb::image

#endif // _JPG_CODEC_HPP_