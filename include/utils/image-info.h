#ifndef _IMAGE_INFO_H_
#define _IMAGE_INFO_H_
#include <cstddef>
#include <cstdlib>
#include <stdint.h>

namespace limb {
struct ImageInfo {
  uint8_t *data;
  size_t size;
  int w;
  int h;
  int c;
};

inline void destroy_image_info(ImageInfo &ii) {
  if (ii.data != nullptr) {
    free(ii.data);
  }
}
} // namespace limb
#endif // _IMAGE_INFO_H_