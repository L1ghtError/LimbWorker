#ifndef _IMAGE_PROCESSOR_H_
#define _IMAGE_PROCESSOR_H_

#include "utils/callbacks.h"
#include "utils/image-info.h"
#include "utils/status.h"

#include <cstdio>
#include <string>

#include <string_view>

// This file describes nessesary parts that image (media) processors should contain

// ---
// Libraries that want to provide a ProcessorModule should do so via a `GetProcessorModule` function
// using C calling convention
// ---

namespace limb {
class ImageProcessor {
public:
  virtual ~ImageProcessor() = default;

  // Get the processor's name used to distinguish it from other processors
  virtual std::string_view name() const = 0;

  virtual liret process_image(const ImageInfo &inimage, ImageInfo &outimage,
                              const ProgressCallback &procb = defaultProgressCallback) const = 0;
};

// Class responsible for providing media processors
class ProcessorContainer {
public:
  virtual ~ProcessorContainer() = default;

  // Allocate and initialize the necessary data for creating media processors
  virtual liret init() = 0;
  virtual liret deinit() = 0;

  // Allocate a media processor; return nullptr if allocation fails
  virtual ImageProcessor *tryAcquireProcessor() = 0;

  // Return a media processor back to the container
  // The container may cache it or deallocate it
  virtual void reclaimProcessor(ImageProcessor *proc) = 0;

  // Get the container's name used to distinguish it from other containers
  virtual std::string_view name() const = 0;
};

// Class that should be returned
class ProcessorModule {
public:
  // Allocate a media processor container; return nullptr if allocation fails
  virtual ProcessorContainer *allocateContainer() = 0;

  // Deallocates a media container.
  virtual void deallocateContainer(ProcessorContainer *proc) = 0;

  // Get the module's name used to distinguish it from other modules
  virtual std::string_view name() const = 0;
};
} // namespace limb
#endif // _IMAGE_PROCESSOR_H_