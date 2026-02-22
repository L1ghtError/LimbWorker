#ifndef _TASK_TYPES_HPP_
#define _TASK_TYPES_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include "utils/status.h"

namespace limb {

struct ImageTask {
  uint32_t modelId;
  std::string imageId;
};

struct PingTask {
  std::string message;
};

struct AppInfoTask {
  struct AvailableProcessor {
    std::string name;
    uint32_t index;
  };

  std::vector<AvailableProcessor> availableProcessors;
  uint32_t totalCpuThreads;
};

} // namespace limb

#endif // _TASK_TYPES_HPP_