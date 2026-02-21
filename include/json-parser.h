#ifndef _JSON_CONFIG_H_
#define _JSON_CONFIG_H_

#include "app-config.h"

namespace limb {
class JsonParser : public ConfigParser {
public:
  JsonParser();
  JsonParser(const std::string &filepath);

  liret getConfig(AppConfig &conf) override;

private:
  std::string m_customFilepath;
};
} // namespace limb

#endif //  _JSON_CONFIG_H_