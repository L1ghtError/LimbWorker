#ifndef _PROCESSOR_LOADER_H_
#define _PROCESSOR_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "image-processor.h"
#include "utils/status.h"

namespace limb {
class ProcessorLoader {
public:
  ProcessorLoader();
  ProcessorLoader(const std::vector<std::string> &additionalDirectories);
  ~ProcessorLoader();

  liret status();

  liret reload();
  liret addLoadDir(const std::string &dirPath);
  liret removeLoadDir(size_t index);

  std::string processorName(size_t index);
  ImageProcessor *allocateProcessor(size_t index);
  void destroyProcessor(ImageProcessor *processor);

  size_t processorCount();
  size_t dirCount();

private:
  class impl;
  std::unique_ptr<impl> pImpl;
};
} // namespace limb

#endif // _PROCESSOR_LOADER_H_