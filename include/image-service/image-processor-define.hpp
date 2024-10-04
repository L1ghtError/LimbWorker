#ifndef _IMAGE_PROCESSOR_DEFINE_HPP_
#define _IMAGE_PROCESSOR_DEFINE_HPP_

#include <stdint.h>

/**
 * Image Processor type
 *
 */
typedef enum {
  IP_IMAGE_NO = 0,
  IP_IMAGE_REALESRGAN, ///< ncnn realesrgan
} IMAGE_PROCESSOR_TYPES;

#endif // _IMAGE_PROCESSOR_DEFINE_HPP_
