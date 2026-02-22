#include "app-tasks/task-parser.hpp"

#include "app-tasks/json-task-parser.hpp"

namespace limb {

TaskParser *TaskParserFactory::fromType(TaskParserType type) {
  switch (type) {
  case TaskParserType::kJson:
    return new (std::nothrow) JsonTaskParser();
    return nullptr;
  case TaskParserType::kProtobuf:
    // return new (std::nothrow) ProtobufTaskParser();
    return nullptr;
  default:
    return nullptr;
  }
}

} // namespace limb