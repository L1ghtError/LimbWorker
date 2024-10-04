#ifndef _DATA_MODELS_HPP_
#define _DATA_MODELS_HPP_
#include "utils/endian.h"
#include "utils/status.h"
#include <cstdint>

namespace limb {

struct MUpscaleImage {
  inline liret berawtoh(const char *raw) {
    *this = *(limb::MUpscaleImage *)(raw);
    modelId = be32toh(modelId);
    return liret::kOk;
  }
  uint32_t modelId;
  uint8_t imageId[24];
};

} // namespace limb
#endif // _DATA_MODELS_HPP_