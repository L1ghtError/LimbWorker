#include "image-service/data-models.hpp"
#include "utils/endian.h"

namespace limb {
liret MUpscaleImage::berawtoh(const char *raw) {
  *this = *(limb::MUpscaleImage *)(raw);
  modelId = be32toh(modelId);
  return liret::kOk;
}
} // namespace limb