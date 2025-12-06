#include "rmbg_1_4_cpu.h"

#include <iostream>

#include "onnxruntime_cxx_api.h"

extern "C" LIMB_API limb::RealesrganProcessor *createProcessor() { return new limb::RMBGProcessor; }

extern "C" LIMB_API void destroyProcessor(limb::ImageProcessor *processor) { delete processor; }

extern "C" LIMB_API const char *processorName() { return "RMBG-1.4"; }

int log_msg() {
  Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
  std::cout << "ONNX Runtime environment created successfully." << std::endl;
  return 0;
}

namespace limb {
RMBGProcessor::RMBGProcessor() {}
RMBGProcessor::~RMBGProcessor() {}
RMBGProcessor::init() {
  log_msg();
  return liret::kUnimplemented;
}
RMBGProcessor::name() { return processorName(); }
RMBGProcessor::load() { return liret::kUnimplemented; }
RMBGProcessor::process_image(const ImageInfo &inimage, ImageInfo &outimage, const ProgressCallback &&procb) const {
  return liret::kUnimplemented;
}
} // namespace limb
