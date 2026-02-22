#include "app-tasks/json-task-parser.hpp"

namespace limb {

liret JsonTaskParser::parse(const uint8_t *data, size_t size, ImageTask &task) {
  if (data == nullptr || size == 0) {
    return liret::kInvalidInput;
  }

  simdjson::padded_string paddedData(reinterpret_cast<const char *>(data), size);

  std::lock_guard<std::mutex> lock(m_parserMutex);
  const simdjson::dom::element doc = m_parser.parse(paddedData);

  auto parsed_uint = doc["modelId"].get_uint64();
  if (parsed_uint.error() || parsed_uint.value() > std::numeric_limits<uint32_t>::max()) {
    return liret::kInvalidInput;
  }
  task.modelId = uint32_t(parsed_uint.value());

  auto parsed = doc["imageId"].get_string();
  if (parsed.error()) {
    return liret::kInvalidInput;
  }
  task.imageId.assign(parsed.value());

  return liret::kOk;
}

liret JsonTaskParser::parse(const uint8_t *data, size_t size, PingTask &task) {
  if (data == nullptr || size == 0) {
    return liret::kInvalidInput;
  }

  simdjson::padded_string paddedData(reinterpret_cast<const char *>(data), size);

  std::lock_guard<std::mutex> lock(m_parserMutex);
  const simdjson::dom::element doc = m_parser.parse(paddedData);

  auto parsed = doc["message"].get_string();
  if (parsed.error()) {
    return liret::kInvalidInput;
  }
  task.message.assign(parsed.value());

  return liret::kOk;
}

liret JsonTaskParser::serialize(std::vector<uint8_t> &data, PingTask &task) {
  std::lock_guard<std::mutex> lock(m_builderMutex);

  m_builder.clear();
  m_builder.start_object();
  m_builder.append_key_value("message", task.message);
  m_builder.end_object();

  data.resize(m_builder.size());

  std::memcpy(reinterpret_cast<void *>(data.data()), m_builder.c_str().value(), m_builder.size());
  return liret::kOk;
}

} // namespace limb