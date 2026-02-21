#include "app-transport/amqp-transport.hpp"

#include "image-service/image-service.hpp"

#include <format>
#include <future>

namespace {
constexpr auto g_pingQueue = "Ping";
constexpr auto g_getAppInfo = "GetAppInfo";
constexpr auto g_processImageQueue = "ProcessImage";

constexpr bool g_consumePing = true;
constexpr auto g_pingResponse = "Pong";
constexpr auto g_processImageCloseMessage = "End";
constexpr auto g_processImageFailMessage = "Fail";

} // namespace

namespace limb {
AmqpTransport::AmqpTransport(const AmqpConfig &conf)
    : m_handler(conf.host.c_str(), conf.port), m_connection(&m_handler, AMQP::Login(conf.user, conf.passwd), "/"),
      m_ch(&m_connection), m_conf(conf) {}

AmqpTransport::~AmqpTransport() { m_handler.quit(); }

liret AmqpTransport::init() {
  m_ch.setQos(m_conf.prefetchCount);
  if (!m_ch.usable()) {
    return liret::kAborted;
  }

  if (g_consumePing) {
    m_ch.consume(g_pingQueue).onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
      // TODO Implement logging with log levels
      std::cout << "[AmqpTransport] Ping: (" << message.body() << ")" << " id:" << message.correlationID() << "\n";

      AMQP::Envelope env(g_pingResponse);
      const std::string repl(message.replyTo());
      env.setCorrelationID(message.correlationID());
      m_ch.publish("", repl, env);
      m_ch.ack(deliveryTag);
    });
  }

  m_ch.consume(g_getAppInfo).onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
    // TODO Implement logging with log levels
    std::cout << "[AmqpTransport] GetCapabilities id:" << message.correlationID() << "\n";

    handleGetAppInfo(message, deliveryTag);
  });

  m_ch.consume(g_processImageQueue)
      .onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        // TODO Implement logging with log levels
        std::cout << "[AmqpTransport] ProcessImage id:" << message.correlationID() << "\n";

        handleProcessImage(message, deliveryTag);
      });

  return liret::kOk;
}

void AmqpTransport::sendResponse(const AMQP::Message &message, uint64_t deliveryTag, const std::vector<uint8_t> &resp) {
  AMQP::Envelope env((const char *)resp.data(), resp.size());
  env.setCorrelationID(message.correlationID());

  const auto repl = message.replyTo();

  std::lock_guard lock(m_chMutex);
  m_ch.publish("", repl, env);
  m_ch.ack(deliveryTag);
}

void AmqpTransport::sendResponse(const AMQP::Message &message, uint64_t deliveryTag, const std::string &resp) {
  AMQP::Envelope env(resp);
  env.setCorrelationID(message.correlationID());

  const auto repl = message.replyTo();

  std::lock_guard lock(m_chMutex);
  m_ch.publish("", repl, env);
  m_ch.ack(deliveryTag);
}

void AmqpTransport::sendReject(uint64_t deliveryTag) { m_ch.reject(deliveryTag); }

AmqpTransportAdapter::AmqpTransportAdapter(const AmqpConfig &conf, const tp::ThreadPoolOptions &options)
    : AmqpTransport(conf), m_app(nullptr), m_pool(options) {}

AmqpTransportAdapter::~AmqpTransportAdapter() = default;

liret AmqpTransportAdapter::init(AppBase *app) {
  if (app == nullptr) {
    return liret::kInvalidInput;
  }
  m_app = app;

  return AmqpTransport::init();
}

void AmqpTransportAdapter::handleGetAppInfo(const AMQP::Message &message, uint64_t deliveryTag) {
  size_t size = 0;
  m_app->getCapabilitiesJSON(nullptr, size);
  std::vector<uint8_t> buf(size);

  m_app->getCapabilitiesJSON(buf.data(), size);

  AmqpTransport::sendResponse(message, deliveryTag, buf);
}

using namespace std::chrono;

void AmqpTransportAdapter::handleProcessImage(const AMQP::Message &message, uint64_t deliveryTag) {
  // Todo parse body
  limb::ImageTask task;

  const auto sendResp = [this, &message, &deliveryTag](const std::string &resp) {
    AmqpTransport::sendResponse(message, deliveryTag, resp);
  };

  const auto progressCb = [this, &sendResp](float value) {
    // TODO use dedicated class to provide response in any format
    const std::string progress = std::format("{\"progress\":.2f}", value);
    // TODO Implement logging with log levels
    std::cout << "[AmqpTransportAdapter] handleProcessImage progress:" << progress << "\n";

    sendResp(progress);
  };

  const auto handler = [this, &task, &progressCb, &sendResp]() {
    liret ret = m_app->processImage(task, progressCb);
    if (ret != liret::kOk) {
      // TODO Implement logging with log levels
      std::cerr << "[AmqpTransportAdapter] handleProcessImage Error:" << listat::getErrorMessage(ret) << "\n";
      sendResp(g_processImageFailMessage);
      return;
    }

    std::cout << "[AmqpTransportAdapter] handleProcessImage Done!\n";

    sendResp(g_processImageCloseMessage);
  };

  std::packaged_task<void()> packaged(handler);
  if (tp.tryPost(packaged) == false) {
    ch.reject(deliveryTag);
  }
}

} // namespace limb

char *extractBody(const AMQP::Message &message) {
  const int bodySize = message.bodySize();
  char *msg = new char[bodySize];
  const char *body = message.body();
  memcpy(msg, body, bodySize);
  return msg;
}

liret setRoutes(limb::AppBase *application, limb::tp::ThreadPool &tp, AMQP::Connection &conn, AMQP::Channel &ch) {
  if (!ch.usable()) {
    return liret::kInvalidInput;
  }

  ch.consume("").onReceived(
      [application, &ch, &tp](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        const char *body = extractBody(message);
        limb::ImageTask pbody;

        std::string correlationID = message.correlationID();
        std::string replyTo = message.replyTo();
        auto handler = [application, correlationID, replyTo, deliveryTag, body, pbody, &ch]() {
          // Progress response callback
          int iteration = 0;
          std::chrono::high_resolution_clock::time_point firstEnt;
          auto progressCb = [correlationID, replyTo, &iteration, &firstEnt, &ch](float progress) {
            if (iteration == 0) {
              firstEnt = std::chrono::high_resolution_clock::now();
            } else if (iteration == 2) {
              char prog[32] = {0};
              float estimation = estimateProgress(firstEnt, progress);
              snprintf(prog, sizeof(prog), "%.2f:%.2fms", progress, estimation);
              printf("Res-%s \n", prog);
              AMQP::Envelope env(prog);

              env.setCorrelationID(correlationID);
              ch.publish("", replyTo, env);
            }
            ++iteration;
          };

          liret ret = application->processImage(pbody, progressCb);
          if (ret != liret::kOk) {
            printf("Err upscale %s\n", listat::getErrorMessage(ret));
          }

          AMQP::Envelope env("End");
          env.setCorrelationID(correlationID);

          ch.publish("", replyTo, env);
          auto currentEnt = std::chrono::high_resolution_clock::now();
          std::chrono::duration<float, std::milli> duration = currentEnt - firstEnt;
          printf("sent %s / %.2fms \n", "End", duration.count());
          ch.ack(deliveryTag);
          delete body;
        };

        std::packaged_task<void()> packedHandler(handler);
        std::future<void> futureHandler = packedHandler.get_future();
        if (tp.tryPost(packedHandler) == false) {
          ch.reject(deliveryTag);
          delete body;
        }
      });

  return liret::kOk;
}