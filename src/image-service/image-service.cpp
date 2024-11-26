#include "image-service/image-service.hpp"

#include "mongo-client/mongo-client.hpp"

#include "ncnn-service/ncnn-service-realesrgan.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "utils/stb-include.h"

namespace limb {

ImageService *imageService = nullptr;

ImageService::~ImageService() {
  // TODO: investigate nessesity of manually manage .net lifetime
  // for (int i = 0; i < m_services.size(); i++) {
  //   delete m_services[i].net;
  // }
}
liret ImageService::handleUpscaleImage(const MUpscaleImage &input, const ProgressCallback &&procb) {
  bson_oid_t imageId;
  // input.imageId Need to be uint8_t[24]
  bson_oid_init_from_string(&imageId, (const char *)input.imageId);
  uint8_t *filedata;
  ssize_t size;

  liret ret = limb::mongoService->getImageById(&imageId, &filedata, &size);
  if (ret != liret::kOk) {
    return ret;
  }

  int w, h, c;
  uint8_t *pixeldata = stbi_load_from_memory(filedata, size, &w, &h, &c, 0);
  delete filedata;
  NcnnService *imgProc;
  imageProcessorNew((IMAGE_PROCESSOR_TYPES)input.modelId, &imgProc);
  if (ret != liret::kOk) {
    return ret;
  }

  ncnn::Mat inimage(w, h, (void *)pixeldata, (size_t)c, c);
  ncnn::Mat outimage(w * 4, h * 4, (size_t)c, c);
  imgProc->process_matrix(inimage, outimage, ProgressCallback(procb));
  delete imgProc;
  delete pixeldata;
  int outSize = 0;
  uint8_t *outImage =
      stbi_write_png_to_mem((uint8_t *)outimage.data, 0, outimage.w, outimage.h, outimage.elempack, &outSize);

  ret = limb::mongoService->updateImageById(&imageId, outImage, outSize);
  if (ret != liret::kOk) {
    return ret;
  }

  return liret::kOk;
}

liret ImageService::addOption(const ImageServiceOptions &opt) {
  // should be only allocated inside this functoin
  int err = 0;
  if (err != 0) {
    return liret::kInvalidInput;
  }

  // Update existing serivce
  for (int i = 0; i < m_services.size(); i++) {
    if (m_services[i].type == opt.type) {
      // if someone pushed unalocated net inside m_services
      // thats mean we have some troubles
      // delete m_services[i].net;
      m_services[i] = opt;
      // m_services[i].net = net;
      return liret::kOk;
    }
  }
  ImageServiceOptions newOpt = opt;
  // newOpt.net = net;
  m_services.push_back(newOpt);

  return liret::kOk;
}

liret ImageService::imageProcessorNew(IMAGE_PROCESSOR_TYPES type, NcnnService **proc) {
  const ImageServiceOptions *optr = nullptr;
  for (const auto &it : m_services) {
    if (it.type == type) {
      optr = &it;
      break;
    }
  }
  if (optr == nullptr) {
    return liret::kNotFound;
  }

  switch (type) {
  case IP_IMAGE_REALESRGAN: {
    RealesrganService *service = new RealesrganService(optr->net, optr->tta_mode);
    service->load();
    service->tilesize = optr->tilesize;
    service->scale = optr->scale;
    service->prepadding = optr->prepadding;
    *proc = service;
    break;
  }
  case IP_IMAGE_NO:
    return liret::kUnknown;
  default:
    return liret::kInvalidInput;
  }
  return liret::kOk;
}

void ImageService::imageProcessorDestroy(NcnnService **recorder) {
  if (*recorder != nullptr) {
    delete recorder;
  }
}
} // namespace limb
