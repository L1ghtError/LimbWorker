#ifndef _AMQP_ROUTER_HPP_
#define _AMQP_ROUTER_HPP_
#include <amqpcpp.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "amqp-handler.hpp"

#include "app-config.h"
#include "limb-app.h"

#include "thread-pool/thread-pool.hpp"
#include "utils/status.h"

namespace limb {

class AmqpTransport {
public:
  AmqpTransport(const AmqpConfig &conf);
  virtual ~AmqpTransport();

  liret loop();
  void quit();

  virtual void handlePing(const AMQP::Message &message, uint64_t deliveryTag) = 0;
  virtual void handleGetAppInfo(const AMQP::Message &message, uint64_t deliveryTag) = 0;
  virtual void handleProcessImage(const AMQP::Message &message, uint64_t deliveryTag) = 0;

protected:
  liret init();

  void sendResponse(const AMQP::Message &message, uint64_t deliveryTag, const std::vector<uint8_t> &resp);
  void sendResponse(const AMQP::Message &message, uint64_t deliveryTag, const std::string &resp);
  void sendReject(uint64_t deliveryTag);

private:
  AmqpHandler m_handler;
  AMQP::Connection m_connection;
  AMQP::Channel m_ch;

  std::mutex m_chMutex;
  const AmqpConfig &m_conf;
};

class AmqpTransportAdapter : public AmqpTransport {
public:
  AmqpTransportAdapter(const AmqpConfig &conf, const tp::ThreadPoolOptions &options);
  ~AmqpTransportAdapter() override;

  liret init(AppBase *app);

  void handlePing(const AMQP::Message &message, uint64_t deliveryTag) override;
  void handleGetAppInfo(const AMQP::Message &message, uint64_t deliveryTag) override;
  void handleProcessImage(const AMQP::Message &message, uint64_t deliveryTag) override;

private:
  AppBase *m_app;
  tp::ThreadPool m_pool;
};

} // namespace limb
#endif // _AMQP_ROUTER_HPP_