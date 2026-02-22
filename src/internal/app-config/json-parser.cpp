#include "json-parser.h"

#include <limits>

#include <simdjson.h>

namespace {
constexpr auto g_defaultConfigPath = "./config.json";

liret tryFillMongoConfig(const simdjson::dom::element &database, limb::MongoConfig &conf) {
  auto parsed = database["type"].get_string();
  if (parsed.error() || parsed.value().compare("mongo") != 0) {
    return liret::kIncomplete;
  }

  parsed = database["uri"].get_string();
  if (parsed.error()) {
    return liret::kIncomplete;
  }
  conf.uri.assign(parsed.value());

  parsed = database["dbName"].get_string();
  if (parsed.error()) {
    return liret::kIncomplete;
  }
  conf.dbName.assign(parsed.value());

  return liret::kOk;
}

void tryDefaultsAmqpTransport(limb::AmqpConfig &conf) {
  if (conf.heartbeat == 0) {
    conf.heartbeat = 60;
  }

  if (conf.prefetchCount == 0) {
    conf.prefetchCount = 20;
  }
}

liret tryFillAmqpTransport(const simdjson::dom::element &transport, limb::AmqpConfig &conf) {
  auto parsed = transport["type"].get_string();
  if (parsed.error() || parsed.value().compare("amqp") != 0) {
    return liret::kIncomplete;
  }

  parsed = transport["user"].get_string();
  if (parsed.error()) {
    return liret::kIncomplete;
  }
  conf.user.assign(parsed.value());

  parsed = transport["passwd"].get_string();
  if (parsed.error()) {
    return liret::kIncomplete;
  }
  conf.passwd.assign(parsed.value());

  parsed = transport["host"].get_string();
  if (parsed.error()) {
    return liret::kIncomplete;
  }
  conf.host.assign(parsed.value());

  auto parsed_uint = transport["port"].get_uint64();
  if (parsed_uint.error() || parsed_uint.value() > std::numeric_limits<uint16_t>::max()) {
    return liret::kIncomplete;
  }
  conf.port = uint16_t(parsed_uint.value());

  return liret::kOk;
}

liret parseDocument(const simdjson::dom::element &doc, limb::AppConfig &conf) {
  liret ret = liret::kOk;

  if (doc["application"].error()) {
    return liret::kIncomplete;
  }

  simdjson::dom::element app = doc["application"];

  if (app["database"].error() == simdjson::SUCCESS) {
    ret = tryFillMongoConfig(app["database"], conf.dbConfig);
  } else {
    ret = liret::kIncomplete;
  }
  if (ret != liret::kOk) {
    return ret;
  }

  if (app["transport"].error() == simdjson::SUCCESS) {
    tryDefaultsAmqpTransport(conf.transportConfig);
    ret = tryFillAmqpTransport(app["transport"], conf.transportConfig);
  } else {
    ret = liret::kIncomplete;
  }

  return ret;
}

} // namespace

namespace limb {
JsonParser::JsonParser() {}

JsonParser::JsonParser(const std::string &filepath) : m_customFilepath(filepath) {}

liret JsonParser::getConfig(AppConfig &conf) {
  simdjson::padded_string json;

  if (!m_customFilepath.empty()) {
    auto res = simdjson::padded_string::load(m_customFilepath);
    if (!res.has_value()) {
      return liret::kIncomplete;
    }

    json = std::move(res.value());
  } else {
    auto res = simdjson::padded_string::load(g_defaultConfigPath);
    if (!res.has_value()) {
      return liret::kIncomplete;
    }

    json = std::move(res.value());
  }

  simdjson::dom::parser parser;
  return parseDocument(parser.parse(json), conf);
}
} // namespace limb