#ifndef _TASK_PARSER_HPP_
#define _TASK_PARSER_HPP_

#include "task-types.hpp"

#include <vector>

namespace limb {

enum class TaskParserType {
  kJson = 0,
  kProtobuf = 1,
};

// parse - bin to struct
// serialize - struct to bin
class TaskParser {
public:
  virtual ~TaskParser() = default;

  virtual liret parse(const uint8_t *data, size_t size, ImageTask &task) = 0;

  virtual liret parse(const uint8_t *data, size_t size, PingTask &task) = 0;
  virtual liret serialize(std::vector<uint8_t> &data, PingTask &task) = 0;
};

class TaskParserFactory {
public:
  static TaskParser *fromType(TaskParserType type);
};

} // namespace limb

#endif // _TASK_PARSER_HPP_