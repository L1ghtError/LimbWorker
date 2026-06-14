#ifndef _IMAGE_TYPES_HPP_
#define _IMAGE_TYPES_HPP_

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "utils/status.h"

namespace limb::image {

using ContainerDataType = uint8_t;
using EncodedDataType = uint8_t;

using ContainerDataDeleter = std::function<void(ContainerDataType[])>;
using EncodedDataDeleter = std::function<void(EncodedDataType[])>;

using ContainerData = std::unique_ptr<ContainerDataType[], ContainerDataDeleter>;
using EncodeData = std::unique_ptr<EncodedDataType, EncodedDataDeleter>;

using EncodeCb = std::function<liret(EncodeData, size_t)>;

enum class CodecType {
  kPng = 0,
  kJpg = 1,
  Count
};

struct Container {
  ContainerData data;
  size_t size;
  int32_t w, h, c;
};

} // namespace limb::image

#endif // _IMAGE_TYPES_HPP_