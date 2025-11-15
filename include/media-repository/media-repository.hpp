#ifndef _MEDIA_REPOSITORY_HPP_
#define _MEDIA_REPOSITORY_HPP_
#include "utils/status.h"

namespace limb {

class MediaRepository {
public:
  virtual ~MediaRepository() = default;

  virtual liret getImageById(const char *id, size_t size, unsigned char **filedata, size_t *filesize) = 0;
  virtual liret updateImageById(const char *id, size_t size, unsigned char *filedata, size_t filesize) = 0;
};
} // namespace limb
#endif // _MEDIA_REPOSITORY_HPP_