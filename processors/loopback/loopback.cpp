#include "loopback/loopback.h"

extern "C" LIMB_API limb::LoopbackModule *createProcessor() { return new limb::LoopbackModule; }

namespace limb {
LoopbackContainer::LoopbackContainer() {};
LoopbackContainer::~LoopbackContainer() {};

liret LoopbackContainer::init() { return liret::kOk; }
liret LoopbackContainer::deinit() { return liret::kOk; }

void LoopbackContainer::reclaimProcessor(ImageProcessor *proc) { delete proc; }
ImageProcessor *LoopbackContainer::tryAcquireProcessor() { return new LoopbackProcessor; }

LoopbackProcessor::LoopbackProcessor() {};
LoopbackProcessor::~LoopbackProcessor() {};

liret LoopbackProcessor::process_image(const ImageInfo &inimage, ImageInfo &outimage,
                                       const ProgressCallback &procb) const {
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