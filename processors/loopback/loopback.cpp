#include "loopback/loopback.h"

extern "C" LIMB_API limb::LoopbackProcessor *createProcessor() { return new limb::LoopbackProcessor; }

extern "C" LIMB_API void destroyProcessor(limb::ImageProcessor *processor) { delete processor; }

extern "C" LIMB_API const char *processorName() { return "Loopback-Processor"; }

namespace limb {
LoopbackProcessor::LoopbackProcessor() {};
LoopbackProcessor::~LoopbackProcessor() {};

liret LoopbackProcessor::init() { return liret::kOk; }
liret LoopbackProcessor::load() { return liret::kOk; }
const char *LoopbackProcessor::name() { return processorName(); }
liret LoopbackProcessor::process_image(const ImageInfo &inimage, ImageInfo &outimage,
                                       const ProgressCallback &&procb) const {
  // Simple copy from input to output
  outimage.w = inimage.w;
  outimage.h = inimage.h;
  outimage.c = inimage.c;
  size_t datasize = inimage.w * inimage.h * inimage.c;
  outimage.data = new unsigned char[datasize];
  std::memcpy(outimage.data, inimage.data, datasize);
  procb(1.0f); // report progress as done
  return liret::kOk;
}
} // namespace limb