#include "app-transport/amqp-transport.hpp"

#include "image-service/image-service.hpp"

#include "app-tasks/task-parser.hpp"

#include <format>
#include <future>

namespace {
constexpr auto g_pingQueue = "Ping";
constexpr auto g_getAppInfo = "GetAppInfo";
constexpr auto g_processImageQueue = "ProcessImage";

constexpr bool g_consumePing = true;
constexpr auto g_pingResponse = "Pong";
constexpr auto g_pingRequestPayload = g_pingQueue;

constexpr auto g_processImageDoneMessage = "Done";
constexpr auto g_processImageFailMessage = "Fail";

} // namespace

namespace limb {
AmqpTransport::AmqpTransport(const AmqpConfig &conf, const tp::ThreadPoolOptions &options)
    : m_handler(conf.host.c_str(), conf.port), m_connection(&m_handler, AMQP::Login(conf.user, conf.passwd), "/"),
      m_ch(&m_connection), m_conf(conf), m_pool(options) {}

AmqpTransport::~AmqpTransport() { m_handler.quit(); }

liret AmqpTransport::loop() {
  m_handler.loop();
  return liret::kOk;
}

void AmqpTransport::quit() { m_handler.quit(); }

liret AmqpTransport::init() {
  m_ch.setQos(m_conf.prefetchCount);
  if (!m_ch.usable()) {
    return liret::kAborted;
  }

  if (g_consumePing) {
    m_ch.declareQueue(g_pingQueue);
    m_ch.consume(g_pingQueue).onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
      // TODO Implement logging with log levels
      std::cout << "[AmqpTransport] Ping id:" << message.correlationID() << "\n";

      handlePing(message, deliveryTag);
    });
  }

  m_ch.declareQueue(g_getAppInfo);
  m_ch.consume(g_getAppInfo).onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
    // TODO Implement logging with log levels
    std::cout << "[AmqpTransport] GetCapabilities id:" << message.correlationID() << "\n";

    handleGetAppInfo(message, deliveryTag);
  });

  m_ch.declareQueue(g_processImageQueue);
  m_ch.consume(g_processImageQueue)
      .onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        // TODO Implement logging with log levels
        std::cout << "[AmqpTransport] ProcessImage id:" << message.correlationID() << "\n";

        const auto first = reinterpret_cast<const uint8_t *>(message.body());
        const auto last = reinterpret_cast<const uint8_t *>(message.body()) + message.bodySize();

        AmqpTask task{.correlationID = message.correlationID(),
                      .replyTo = message.replyTo(),
                      .deliveryTag = deliveryTag,
                      .body{first, last}};

        std::packaged_task<void()> packaged([this, t = std::move(task)]() mutable { handleProcessImage(t); });
        if (m_pool.tryPost(packaged) == false) {
          sendReject(deliveryTag);
        }
      });

  return liret::kOk;
}

void AmqpTransport::sendResponse(AmqpTask &task, const std::vector<uint8_t> &resp) {
  AMQP::Envelope env((const char *)resp.data(), resp.size());
  env.setCorrelationID(task.correlationID);

  const auto &repl = task.replyTo;

  std::lock_guard lock(m_chMutex);
  if (!m_ch.publish("", repl, env)) {
    std::cerr << "[AmqpTransport] sendResponse (AmqpTask) Failed to send task! id:" << task.correlationID << "\n";
  }
}

void AmqpTransport::sendResponse(const AMQP::Message &message, uint64_t deliveryTag, const std::vector<uint8_t> &resp) {
  AMQP::Envelope env((const char *)resp.data(), resp.size());
  env.setCorrelationID(message.correlationID());

  const auto &repl = message.replyTo();

  std::lock_guard lock(m_chMutex);
  m_ch.publish("", repl, env);
  m_ch.ack(deliveryTag);
}

void AmqpTransport::sendResponse(const AMQP::Message &message, uint64_t deliveryTag, const std::string &resp) {
  AMQP::Envelope env(resp);
  env.setCorrelationID(message.correlationID());

  const auto &repl = message.replyTo();

  std::lock_guard lock(m_chMutex);
  m_ch.publish("", repl, env);
  m_ch.ack(deliveryTag);
}

void AmqpTransport::sendReject(uint64_t deliveryTag) { m_ch.reject(deliveryTag); }

void AmqpTransport::sendAck(uint64_t deliveryTag) {
  if (!m_ch.ack(deliveryTag)) {
    // TODO Implement logging with log levels
    std::cerr << "[AmqpTransport] sendResponse (AmqpTask) Failed to send ack! tag:" << deliveryTag << "\n";
  }
}

AmqpTransportAdapter::AmqpTransportAdapter(const AmqpConfig &conf, const tp::ThreadPoolOptions &options)
    : AmqpTransport(conf, options), m_app(nullptr) {}

AmqpTransportAdapter::~AmqpTransportAdapter() = default;

liret AmqpTransportAdapter::init(AppBase *app) {
  if (app == nullptr) {
    return liret::kInvalidInput;
  }
  m_app = app;

  return AmqpTransport::init();
}

void AmqpTransportAdapter::handlePing(const AMQP::Message &message, uint64_t deliveryTag) {
  std::unique_ptr<TaskParser> taskParser(TaskParserFactory::fromType(TaskParserType::kJson));

  limb::PingTask task;
  if (taskParser == nullptr ||
      taskParser->parse((const uint8_t *)message.body(), message.bodySize(), task) != liret::kOk) {
    // TODO Implement logging with log levels
    std::cerr << "[AmqpTransportAdapter] handlePing Failed to parse task! id:" << message.correlationID() << "\n";
    sendReject(deliveryTag);
    return;
  }

  if (task.message != g_pingRequestPayload) {
    // TODO Implement logging with log levels
    std::cerr << "[AmqpTransportAdapter] handlePing Failed to parse task! id:" << message.correlationID() << "\n";
    sendReject(deliveryTag);
    return;
  }

  std::cout << "[AmqpTransportAdapter] handlePing (" << task.message << ")\n";

  task.message = g_pingResponse;

  std::vector<uint8_t> response;
  if (taskParser->serialize(response, task) != liret::kOk) {
    // TODO Implement logging with log levels
    std::cerr << "[AmqpTransportAdapter] handlePing Failed to serialize task! id:" << message.correlationID() << "\n";
    sendReject(deliveryTag);
    return;
  }

  sendResponse(message, deliveryTag, response);
}

void AmqpTransportAdapter::handleGetAppInfo(const AMQP::Message &message, uint64_t deliveryTag) {
  std::unique_ptr<TaskParser> taskParser(TaskParserFactory::fromType(TaskParserType::kJson));

  AppInfoTask task = m_app->getAppInfo();

  std::vector<uint8_t> response;
  if (taskParser == nullptr || taskParser->serialize(response, task) != liret::kOk) {
    // TODO Implement logging with log levels
    std::cerr << "[AmqpTransportAdapter] handleGetAppInfo Failed to serialize task! id:" << message.correlationID()
              << "\n";
    sendReject(deliveryTag);
    return;
  }

  sendResponse(message, deliveryTag, response);
}

using namespace std::chrono;

void AmqpTransportAdapter::handleProcessImage(AmqpTask &message) {
  std::unique_ptr<TaskParser> taskParser(TaskParserFactory::fromType(TaskParserType::kJson));

  limb::ImageTask task;
  if (taskParser == nullptr ||
      taskParser->parse((const uint8_t *)message.body.data(), message.body.size(), task) != liret::kOk) {
    // TODO Implement logging with log levels
    std::cerr << "[AmqpTransportAdapter] handleProcessImage Failed to parse task! id:" << message.correlationID << "\n";
    sendReject(message.deliveryTag);
    return;
  }

  auto sendRespVec = [this, &message](const std::vector<uint8_t> &resp) { AmqpTransport::sendResponse(message, resp); };

  auto progressCb = [this, &taskParser, &message, &sendRespVec](float value) {
    // TODO use dedicated class to provide response in any format
    const std::string progress = std::format("{:.2f}", value);
    ;
    // TODO Implement logging with log levels
    std::cout << "[AmqpTransportAdapter] handleProcessImage progress:" << progress << "\n";

    std::vector<uint8_t> response;
    if (taskParser->serialize(response, ImageTaskResult{.message = progress,
                                                        .status = ImageTaskResult::Status::Progress}) != liret::kOk) {
      sendReject(message.deliveryTag);
      return;
    }

    sendRespVec(response);
  };

  liret ret = m_app->processImage(task, progressCb);
  if (ret != liret::kOk) {
    // TODO Implement logging with log levels
    std::cerr << "[AmqpTransportAdapter] handleProcessImage Error:" << listat::getErrorMessage(ret) << "\n";

    std::vector<uint8_t> response;
    if (taskParser->serialize(response, ImageTaskResult{.message = g_processImageFailMessage,
                                                        .status = ImageTaskResult::Status::Fail}) != liret::kOk) {
      sendReject(message.deliveryTag);
      return;
    }

    sendRespVec(response);
    return;
  }

  std::cout << "[AmqpTransportAdapter] handleProcessImage Done!\n";

  std::vector<uint8_t> response;
  if (taskParser->serialize(response, ImageTaskResult{.message = g_processImageDoneMessage,
                                                      .status = ImageTaskResult::Status::Done}) != liret::kOk) {
    sendReject(message.deliveryTag);
    return;
  }

  sendRespVec(response);
  sendAck(message.deliveryTag);
}

} // namespace limb