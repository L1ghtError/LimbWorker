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

struct AmqpTask {
  const std::string correlationID;
  const std::string replyTo;

  uint64_t deliveryTag;

  std::vector<uint8_t> body;
};

class AmqpTransport {
public:
  AmqpTransport(const AmqpConfig &conf, const tp::ThreadPoolOptions &options);
  virtual ~AmqpTransport();

  liret loop();
  void quit();

  virtual void handlePing(const AMQP::Message &message, uint64_t deliveryTag) = 0;
  virtual void handleGetAppInfo(const AMQP::Message &message, uint64_t deliveryTag) = 0;
  virtual void handleProcessImage(AmqpTask &message) = 0;

protected:
  liret init();

  // Sends a response using a task state object.
  // This version is intended for scenarios where multiple messages may be sent using the same task.
  // Note: The caller is responsible for manually acknowledging the task after sending.
  void sendResponse(AmqpTask &task, const std::vector<uint8_t> &resp);

  // Sends a response to a specific AMQP message and automatically acknowledges it.
  void sendResponse(const AMQP::Message &message, uint64_t deliveryTag, const std::vector<uint8_t> &resp);
  void sendResponse(const AMQP::Message &message, uint64_t deliveryTag, const std::string &resp);

  void sendReject(uint64_t deliveryTag);
  void sendAck(uint64_t deliveryTag);

private:
  AmqpHandler m_handler;
  AMQP::Connection m_connection;
  AMQP::Channel m_ch;

  std::mutex m_chMutex;
  const AmqpConfig &m_conf;

  tp::ThreadPool m_pool;
};

class AmqpTransportAdapter : public AmqpTransport {
public:
  AmqpTransportAdapter(const AmqpConfig &conf, const tp::ThreadPoolOptions &options);
  ~AmqpTransportAdapter() override;

  liret init(AppBase *app);

  void handlePing(const AMQP::Message &message, uint64_t deliveryTag) override;
  void handleGetAppInfo(const AMQP::Message &message, uint64_t deliveryTag) override;
  void handleProcessImage(AmqpTask &message) override;

private:
  AppBase *m_app;
};

} // namespace limb
#endif // _AMQP_ROUTER_HPP_