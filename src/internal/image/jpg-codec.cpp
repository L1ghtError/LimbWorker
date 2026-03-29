#include "image/jpg-codec.hpp"

#include <turbojpeg.h>

#include <array>

namespace {

constexpr std::array<unsigned char, 2> jpegSignatureStart = {0xFF, 0xD8};
constexpr std::array<unsigned char, 2> jpegSignatureEnd = {0xFF, 0xD9};

constexpr int kCompressQuality = 100;

void commonDeleter(void *ptr) {
  if (!ptr) {
    return;
  }
  tjFree((unsigned char *)ptr);
}
} // namespace

namespace limb::image {

JpgCodec::JpgCodec() : m_jpegCompressor(nullptr, _tjDeleter), m_jpegDecompressor(nullptr, _tjDeleter) {}

JpgCodec::~JpgCodec() {}

void JpgCodec::_tjDeleter(void *ptr) { tjDestroy(ptr); }

bool JpgCodec::canDecode(std::span<const unsigned char> encoded) {
  if (encoded.size() < jpegSignatureStart.size() + jpegSignatureEnd.size())
    return false;

  for (size_t i = 0; i < jpegSignatureStart.size(); ++i) {
    if (encoded[i] != jpegSignatureStart[i])
      return false;
  }

  const size_t endOffset = encoded.size() - jpegSignatureEnd.size();
  for (size_t i = 0; i < jpegSignatureEnd.size(); ++i) {
    if (encoded[endOffset + i] != jpegSignatureEnd[i])
      return false;
  }

  return true;
}

liret JpgCodec::decode(std::span<const EncodedDataType> encoded, Container &container) {
  if (!canDecode(encoded)) {
    return liret::kInvalidInput;
  }

  if (!m_jpegDecompressor) {
    m_jpegDecompressor.reset(tjInitDecompress());
  }

  int jpegSubsamp, w, h, c;
  if (tjDecompressHeader2(m_jpegDecompressor.get(), encoded.data(), encoded.size(), &w, &h, &jpegSubsamp) != 0) {
    return liret::kInvalidInput;
  }

  TJPF format;
  if (jpegSubsamp == TJSAMP_GRAY) [[unlikely]] {
    format = TJPF_GRAY;
    c = 1;
  } else [[likely]] {
    format = TJPF_RGB;
    c = 3;
  }

  container.data.reset(new (std::nothrow) ContainerDataType[w * h * c]);
  container.data.get_deleter() = std::default_delete<ContainerDataType[]>();
  if (!container.data) {
    return liret::kOutOfMemory;
  }

  if (tjDecompress2(m_jpegDecompressor.get(), encoded.data(), encoded.size(), container.data.get(), w, 0, h, format,
                    TJFLAG_ACCURATEDCT) != 0) {
    return liret::kInvalidInput;
  }

  container.size = w * h * c;
  container.w = w;
  container.h = h;
  container.c = c;

  return liret::kOk;
};

liret JpgCodec::encode(const Container &container, EncodeCb cb) {
  if (!container.data) {
    return liret::kInvalidInput;
  }

  if (!m_jpegCompressor) {
    m_jpegCompressor.reset(tjInitCompress());
  }

  TJPF format;
  if (container.c == 3) [[likely]] {
    format = TJPF_RGB;
  } else [[unlikely]] {
    format = TJPF_GRAY;
  }

  unsigned long outSize;
  unsigned char *jpegBuf;
  tjCompress2(m_jpegCompressor.get(), container.data.get(), container.w, 0, container.h, format, &jpegBuf, &outSize,
              TJSAMP_444, kCompressQuality, TJFLAG_FASTDCT);
  if (!jpegBuf) {
    return liret::kAborted;
  }

  EncodeData compressed(jpegBuf, commonDeleter);
  if (!compressed) {
    return liret::kOutOfMemory;
  }

  return cb(std::move(compressed), outSize);
};

} // namespace limb::image