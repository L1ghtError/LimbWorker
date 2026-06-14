#ifndef _IMAGE_CODEC_HPP_
#define _IMAGE_CODEC_HPP_

#include "image-types.h"

#include <array>
#include <memory>
#include <mutex>
#include <span>

namespace limb::image {

class Codec {
public:
  virtual ~Codec() = default;

  virtual liret decode(std::span<const EncodedDataType> encoded, Container &container) = 0;

  virtual liret encode(const Container &container, EncodeCb cb) = 0;

  virtual CodecType type() const = 0;
};

enum class AllocationType { Initial = 0, Lazy = 1 };

class CodecFactory {
public:
  [[nodiscard]]
  static CodecFactory *getInstance();

  static void reclaimCodec(Codec *codec);

  using reclaim = decltype(&reclaimCodec);

  [[nodiscard]]
  std::unique_ptr<Codec, reclaim> acquireFromType(CodecType type, AllocationType at = AllocationType::Lazy);

  [[nodiscard]]
  std::unique_ptr<Codec, reclaim> acquireFromData(std::span<const EncodedDataType> encoded,
                                                  AllocationType at = AllocationType::Lazy);

private:
  static constexpr size_t to_index(CodecType t) { return static_cast<size_t>(t); }

  void _reclaimCodec(Codec *codec);

  std::array<std::vector<Codec *>, size_t(CodecType::Count)> m_pool;
  std::mutex m_pool_mtx;
};

} // namespace limb::image

#endif // _IMAGE_CODEC_HPP_