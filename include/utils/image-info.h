#ifndef _IMAGE_INFO_H_
#define _IMAGE_INFO_H_
#include <cstddef>
#include <cstdlib>
#include <stdint.h>

namespace limb {
struct ImageInfo {
  uint8_t *data;
  size_t size;
  uint32_t w;
  uint32_t h;
  uint32_t c;
};

inline void destroy_image_info(ImageInfo &ii) {
  if (ii.data != nullptr) {
    free(ii.data);
  }
}
} // namespace limb
#endif // _IMAGE_INFO_H_