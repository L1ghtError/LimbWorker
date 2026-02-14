#ifndef _RMBG_PROCESSOR_H_
#define _RMBG_PROCESSOR_H_

#include "image-processor.h"

#ifdef _WIN32
#ifdef RMBG_EXPORTS
#include <windows.h>
#define LIMB_API __declspec(dllexport)
#else
#define LIMB_API __declspec(dllimport)
#endif
#else
#define LIMB_API
#endif

#include "onnxruntime_cxx_api.h"

#include <layer.h>
#include <pipeline.h>

#include <cstdint>
#include <vector>

namespace limb {
class LIMB_API RmbgProcessor : public ImageProcessor {
public:
  RmbgProcessor();
  ~RmbgProcessor() override;

  liret init() override;

  liret load() override;

  const char *name() override;

  liret process_image(
      const ImageInfo &inimage, ImageInfo &outimage,
      const ProgressCallback &&procb = [](float val) { printf("%.2f\n", val); }) const override;

private:
  Ort::RunOptions runOptions;
  Ort::Env env;
  Ort::SessionOptions sessionOptions;
  Ort::MemoryInfo memInfo;
  Ort::Session *session{nullptr};

  OrtCUDAProviderOptions cudaOptions;

  ncnn::Pipeline *rmbg_preproc{nullptr};
  ncnn::Pipeline *rmbg_postproc{nullptr};
  ncnn::VulkanDevice *vulkanDevice{nullptr};

  std::vector<uint8_t> modelBuffer;

  int gpuIndex;
};
} // namespace limb

extern "C" LIMB_API limb::RmbgProcessor *createProcessor();

extern "C" LIMB_API void destroyProcessor(limb::ImageProcessor *processor);

extern "C" LIMB_API const char *processorName();

#endif // _RMBG_PROCESSOR_H_
