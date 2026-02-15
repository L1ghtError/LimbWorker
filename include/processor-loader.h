#ifndef _PROCESSOR_LOADER_H_
#define _PROCESSOR_LOADER_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "processor-module.h"
#include "utils/status.h"

namespace limb {
class ProcessorLoader {
public:
  ProcessorLoader();
  ProcessorLoader(const std::vector<std::string> &additionalDirectories);
  ProcessorLoader(ProcessorLoader &&pl);
  ~ProcessorLoader();

  liret status();

  liret reload();
  liret addLoadDir(const std::string &dirPath);
  liret removeLoadDir(size_t index);

  std::string_view processorName(size_t index);
  ProcessorContainer *allocateContainer(size_t index);
  void destroyContainer(ProcessorContainer *container);

  size_t processorCount() const;
  size_t dirCount() const;

private:
  class impl;
  std::unique_ptr<impl> pImpl;
};
} // namespace limb

#endif // _PROCESSOR_LOADER_H_