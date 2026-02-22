#ifndef _TASK_TYPES_HPP_
#define _TASK_TYPES_HPP_

#include <cstdint>
#include <string>

#include "utils/status.h"

namespace limb {

struct ImageTask {
  uint32_t modelId;
  std::string imageId;
};

struct PingTask {
  std::string message;
};

} // namespace limb

#endif // _TASK_TYPES_HPP_