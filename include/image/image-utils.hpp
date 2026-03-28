#ifndef _IMAGE_UTILS_HPP_
#define _IMAGE_UTILS_HPP_

#include "image-types.h"

#include <array>
#include <span>

namespace {
constexpr std::array<unsigned char, 8> pngSignature = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
}

namespace limb::image {
inline bool isPng(std::span<const EncodedDataType> encoded) {
  if (encoded.size() < pngSignature.size())
    return false;

  for (size_t i = 0; i < pngSignature.size(); ++i) {
    if (encoded[i] != pngSignature[i])
      return false;
  }

  return true;
}
} // namespace limb::image
#endif // _IMAGE_UTILS_HPP_
