#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "utils/stb-wrap.h"

unsigned char *lib_image_write_png_to_mem(const unsigned char *pixels, int stride_bytes, int x, int y, int n,
                                          int *out_len) {
  return stbi_write_png_to_mem(pixels, stride_bytes, x, y, n, out_len);
}

int lib_image_write_png(char const *filename, int x, int y, int comp, const void *data, int stride_bytes) {
  return stbi_write_png(filename, x, y, comp, data, stride_bytes);
}