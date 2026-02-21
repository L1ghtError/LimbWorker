#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "utils/status.h"

namespace limb {

struct ProcessorModules {
  std::vector<std::string> customScanDirectories;
};

struct MongoConfig {
  std::string uri;
  std::string dbName;
};

struct AmqpConfig {
  std::string user;
  std::string passwd;
  std::string host;

  uint16_t port{};
  uint16_t heartbeat{};

  uint16_t prefetchCount{};
};

struct AppConfig {
  MongoConfig dbConfig;
  AmqpConfig transportConfig;
  ProcessorModules modules;
};

class ConfigParser {
public:
  virtual ~ConfigParser() = default;
  virtual liret getConfig(AppConfig &conf) = 0;
};

class ConfigFactory {
public:
  ConfigFactory();
  ConfigFactory(int argc, char **argv);
  ~ConfigFactory();

  liret getConfig(AppConfig &conf);

private:
  std::unique_ptr<ConfigParser> m_parser;
};

} // namespace limb

#endif // _APP_CONFIG_H_