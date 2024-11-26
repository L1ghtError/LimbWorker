#ifndef _DATA_MODELS_HPP_
#define _DATA_MODELS_HPP_
#include "utils/status.h"
#include <cstdint>

namespace limb {

struct MUpscaleImage {
  liret berawtoh(const char *raw);
  uint32_t modelId;
  uint8_t imageId[24];
};

} // namespace limb
#endif // _DATA_MODELS_HPP_