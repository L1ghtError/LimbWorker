#ifndef _STB_INCLUDE_H_
#define _STB_INCLUDE_H_

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
// #define STBI_NO_STDIO
#include "stb-image/stb_image.h"
#include "stb-image/stb_image_write.h"

// Just a wrapper for stb_image and stb_image_write to prevent multiple definig opf STB_IMAGE_WRITE_IMPLEMENTATION
unsigned char *lib_image_write_png_to_mem(const unsigned char *pixels, int stride_bytes, int x, int y, int n,
                                          int *out_len);

int lib_image_write_png(char const *filename, int x, int y, int comp, const void *data, int stride_bytes);

#endif // _STB_INCLUDE_H_