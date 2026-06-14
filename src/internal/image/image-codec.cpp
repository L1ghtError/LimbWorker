#include "image/image-codec.hpp"

#include "image/jpg-codec.hpp"
#include "image/png-codec.hpp"

namespace limb::image {

CodecFactory *CodecFactory::getInstance() {
  static CodecFactory instance;
  return &instance;
}

std::unique_ptr<Codec, CodecFactory::reclaim> CodecFactory::acquireFromType(CodecType type, AllocationType at) {
  std::lock_guard lock{m_pool_mtx};

  auto &poolEntry = m_pool[to_index(type)];
  if (!poolEntry.empty()) {
    Codec *codec = poolEntry.back();
    poolEntry.pop_back();
    return std::unique_ptr<Codec, CodecFactory::reclaim>{codec, &CodecFactory::reclaimCodec};
  }

  switch (type) {
  case CodecType::kPng:
    return std::unique_ptr<Codec, CodecFactory::reclaim>(new PngCodec(), &CodecFactory::reclaimCodec);

  case CodecType::kJpg:
    return std::unique_ptr<Codec, CodecFactory::reclaim>(new JpgCodec(), &CodecFactory::reclaimCodec);

  default:
    return {nullptr, &CodecFactory::reclaimCodec};
  }
}

std::unique_ptr<Codec, CodecFactory::reclaim> CodecFactory::acquireFromData(std::span<const EncodedDataType> encoded,
                                                                            AllocationType at) {
  if (PngCodec::canDecode(encoded)) {
    return acquireFromType(CodecType::kPng, at);
  } else if (JpgCodec::canDecode(encoded)) {
    return acquireFromType(CodecType::kJpg, at);
  }

  return {nullptr, &CodecFactory::reclaimCodec};
}

void CodecFactory::reclaimCodec(Codec *codec) {
  auto instance = getInstance();
  instance->_reclaimCodec(codec);
}

void CodecFactory::_reclaimCodec(Codec *codec) {
  std::lock_guard lock{m_pool_mtx};

  m_pool[to_index(codec->type())].emplace_back(codec);
}
} // namespace limb::image