#ifndef _TASK_PARSER_JSON_HPP_
#define _TASK_PARSER_JSON_HPP_

#include "task-parser.hpp"

#include <mutex>

#include <simdjson.h>

namespace limb {

class JsonTaskParser : public TaskParser {
public:
  liret parse(const uint8_t *data, size_t size, ImageTask &task) override;

  liret parse(const uint8_t *data, size_t size, PingTask &task) override;
  liret serialize(std::vector<uint8_t>& data, PingTask &task) override;

private:
  simdjson::dom::parser m_parser;
  simdjson::builder::string_builder m_builder;

  std::mutex m_parserMutex;
  std::mutex m_builderMutex;
};

} // namespace limb

#endif // _IMAGE_TASK_PARSER_JSON_HPP_