#include "app-config.h"

#include "json-parser.h"

#include <filesystem>

namespace {
constexpr auto g_jsonLocationPattern = "--jsonFilepath=";

bool isValidFile(const std::string &path) {
  std::filesystem::path filePath(path);
  return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath);
}

std::string getJsonFilepath(const std::string &arg) {
  std::string jsonFilepath;

  if (arg.rfind(g_jsonLocationPattern, 0) == 0) {
    jsonFilepath = arg.substr(strlen(g_jsonLocationPattern));

    if (isValidFile(jsonFilepath)) {
      return jsonFilepath;
    }
  }

  return {};
}

} // namespace

namespace limb {

ConfigFactory::ConfigFactory() : m_parser(std::make_unique<JsonParser>()) {}
ConfigFactory::ConfigFactory(int argc, char **argv) {
  std::string jsonFilepath;
  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];

    jsonFilepath = getJsonFilepath(arg);
    if (!jsonFilepath.empty()) {
      break;
    }
  }

  if (jsonFilepath.empty()) {
    m_parser = std::make_unique<JsonParser>();
  } else {
    m_parser = std::make_unique<JsonParser>(jsonFilepath);
  }
}

ConfigFactory::~ConfigFactory() = default;

liret ConfigFactory::getConfig(AppConfig &conf) { return m_parser->getConfig(conf); }

} // namespace limb