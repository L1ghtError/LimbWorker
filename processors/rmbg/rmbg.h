#ifndef _RMBG_PROCESSOR_H_
#define _RMBG_PROCESSOR_H_

#include "processor-module.h"

#ifdef _WIN32
#ifdef RMBG_EXPORTS
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

constexpr auto g_processorName = "RMBG-1.4";

class RmbgProcessor : public ImageProcessor {
public:
  RmbgProcessor(Ort::MemoryInfo &memInfo, Ort::Session *session, int gpugpuIdxIdx);
  ~RmbgProcessor() override;

  std::string_view name() const override { return g_processorName; };

  liret process_image(const ImageInfo &inimage, ImageInfo &outimage,
                      const ProgressCallback &procb = defaultProgressCallback) const override;

private:
  Ort::RunOptions runOptions;

  Ort::MemoryInfo &memInfo;
  Ort::Session *session;

  ncnn::Pipeline *rmbg_postproc{nullptr};
  ncnn::VulkanDevice *vulkanDevice{nullptr};

  int gpuIndex;
};

class RmbgContainer : public ProcessorContainer {
public:
  RmbgContainer();
  ~RmbgContainer() override;

  liret init() override;
  liret deinit() override;

  ImageProcessor *tryAcquireProcessor() override;
  void reclaimProcessor(ImageProcessor *proc) override;

  std::string_view name() const override { return g_processorName; };

private:
  Ort::Env env;
  Ort::MemoryInfo memInfo;

  Ort::Session *session{nullptr};
  Ort::SessionOptions sessionOptions;
  OrtCUDAProviderOptions cudaOptions;

  std::vector<uint8_t> modelBuffer;
};

class LIMB_API RmbgModule : public ProcessorModule {
public:
  ProcessorContainer *allocateContainer() override { return new RmbgContainer; }

  void deallocateContainer(ProcessorContainer *proc) override { delete proc; }

  std::string_view name() const override { return g_processorName; }
};

} // namespace limb

extern "C" LIMB_API limb::RmbgModule *GetProcessorModule();

#endif // _RMBG_PROCESSOR_H_
