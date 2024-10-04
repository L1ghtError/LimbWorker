#ifndef _IMAGE_SERVICE_HPP_
#define _IMAGE_SERVICE_HPP_
#include "image-service/data-models.hpp"
#include "image-service/image-processor-define.hpp"
#include "ncnn-service/ncnn-service.h"
#include "utils/status.h"

#include <vector>

namespace limb {
class ImageService;
struct ImageServiceOptions {
private:
  ncnn::Net *net;

public:
  const char *paramfullpath;
  const char *modelfullpath;
  IMAGE_PROCESSOR_TYPES type;
  int vulkan_device_index;
  int prepadding;
  int tilesize;
  int scale;
  bool tta_mode;
  friend ImageService;
};

class ImageService {
public:
  ~ImageService();

  liret handleUpscaleImage(const MUpscaleImage &input, const ProgressCallback &&procb = [](float val) {});
  // Alocates part of memory that shared between services with same type
  // Also should free memory inside m_services
  liret addOption(const ImageServiceOptions &opt);

  // Allocates new processos of specific type
  liret imageProcessorNew(IMAGE_PROCESSOR_TYPES type, NcnnService **proc);

  void imageProcessorDestroy(NcnnService **recorder);

private:
  std::vector<ImageServiceOptions> m_services;
};
extern ImageService *imageService;
} // namespace limb

#endif // _IMAGE_SERVICE_HPP_