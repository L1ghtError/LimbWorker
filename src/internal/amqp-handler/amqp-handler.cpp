#include "amqp-handler/amqp-handler.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <mutex>
#include <thread>

#include "abnet/abnet.hpp"

#define CONNECTION_TIMEOUT 5000   // 5 seconds
#define HEARTBEAT_RESOLUTION 1000 // 1 second
#define DEFAULT_HEARTBEAT 10      // 10 seconds
class Buffer {
public:
  Buffer(size_t size) : m_data(size, 0), m_use(0) {}

  size_t write(const char *data, size_t size) {
    if (m_use == m_data.size()) {
      return 0;
    }

    const size_t length = (size + m_use);
    size_t write = length < m_data.size() ? size : m_data.size() - m_use;
    memcpy(m_data.data() + m_use, data, write);
    m_use += write;
    return write;
  }

  void drain() { m_use = 0; }

  size_t available() const { return m_use; }

  const char *data() const { return m_data.data(); }

  void shl(size_t count) {

    const size_t diff = m_use - count;
    std::memmove(m_data.data(), m_data.data() + count, diff);
    m_use = m_use - count;
  }

private:
  std::vector<char> m_data;
  size_t m_use;
};

struct AmqpHandlerImpl {
  AmqpHandlerImpl(const AmqpConfig *_conf = nullptr)
      : connected(false), connection(nullptr), quit(false), keepAlive(true), inputBuffer(AmqpHandler::BUFFER_SIZE),
        outBuffer(AmqpHandler::BUFFER_SIZE), tmpBuff(AmqpHandler::TEMP_BUFFER_SIZE), sock(abnet::invalid_socket),
        conf(_conf) {

    if (conf == nullptr) {
      static const AmqpConfig defaultConf = {60};
      conf = &defaultConf;
    }
  }

  Buffer inputBuffer;
  Buffer outBuffer;
  abnet::socket_type sock;
  AMQP::Connection *connection;
  std::vector<char> tmpBuff;

  const AmqpConfig *conf;

  std::atomic<bool> quit;
  std::atomic<std::chrono::high_resolution_clock::time_point> lastMessage;
  // for socket send
  std::mutex sendMtx;
  // for buffer write
  std::mutex writeMtx;
  // for cleaunp
  std::vector<std::future<void>> heartbeaters;

  int keepAlive = true;
  int reuseAddr = true;
  bool connected;
};

AmqpHandler::AmqpHandler(const char *host, uint16_t port, const AmqpConfig *conf) : m_impl(new AmqpHandlerImpl(conf)) {
  int err = 0;
  abnet::error_code ec;
  do {
    abnet::sockaddr_in4_type service;
    // Sock creation
    service.sin_family = ABNET_OS_DEF(AF_INET);
    service.sin_port = abnet::socket_ops::host_to_network_short(port);
    abnet::socket_ops::inet_pton(ABNET_OS_DEF(AF_INET), host, &service.sin_addr, 0, ec);
    if (ec.value() != 0) {
      break;
    }
    m_impl->sock =
        abnet::socket_ops::socket(service.sin_family, ABNET_OS_DEF(SOCK_STREAM), ABNET_OS_DEF(IPPROTO_TCP), ec);
    if (ec.value() != 0) {
      break;
    }

    abnet::socket_ops::setsockopt(m_impl->sock, 0, ABNET_OS_DEF(SOL_SOCKET), ABNET_OS_DEF(SO_REUSEADDR),
                                  &m_impl->reuseAddr, sizeof(m_impl->reuseAddr), ec);
    if (ec.value() != 0) {
      break;
    }

    abnet::socket_ops::setsockopt(m_impl->sock, 0, ABNET_OS_DEF(SOL_SOCKET), ABNET_OS_DEF(SO_KEEPALIVE),
                                  &m_impl->keepAlive, sizeof(m_impl->keepAlive), ec);
    if (ec.value() != 0) {
      break;
    }

    abnet::socket_ops::connect(m_impl->sock, reinterpret_cast<abnet::socket_addr_type *>(&service), sizeof(service),
                               ec);
    if (ec.value() != 0) {
      break;
    }

    return;
  } while (0);
  if (m_impl->sock != abnet::invalid_socket) {
    abnet::socket_ops::close(m_impl->sock, 0, 0, ec);
  }
  m_impl->sock = abnet::invalid_socket;
}

void AmqpHandler::loop() {
  int poll_res = 0;
  abnet::error_code ec;

  while (!m_impl->quit) {

    poll_res = abnet::socket_ops::poll_read(m_impl->sock, 0, 10000, ec);
    if (poll_res == abnet::socket_error_retval) {
      abnet::socket_ops::get_last_error(ec, 0);
      printf("Select error: %s\n", ec.message().c_str());
      break;
    } else {
      // Check if data is available for reading
      size_t bytesAvailable = m_impl->tmpBuff.size();

      if (m_impl->tmpBuff.size() < bytesAvailable) {
        m_impl->tmpBuff.resize(bytesAvailable, 0);
      }
      abnet::signed_size_type bytesRead =
          abnet::socket_ops::recv1(m_impl->sock, &m_impl->tmpBuff[0], bytesAvailable, 0, ec);
      if (bytesRead > bytesAvailable || bytesRead <= 0) {
        printf("Recv error: %s\n", ec.message().c_str());
        break;
      }

      m_impl->inputBuffer.write(m_impl->tmpBuff.data(), bytesRead);
    }

    if (m_impl->connection && m_impl->inputBuffer.available()) {
      size_t count = m_impl->connection->parse(m_impl->inputBuffer.data(), m_impl->inputBuffer.available());

      if (count == m_impl->inputBuffer.available()) {
        m_impl->inputBuffer.drain();
      } else if (count > 0) {
        m_impl->inputBuffer.shl(count);
      }
    }
    sendDataFromBuffer();
  }

  if (m_impl->quit == 0) {
    printf("Network loop force quit!\n");
  }
  if (m_impl->quit && m_impl->outBuffer.available()) {
    sendDataFromBuffer();
  }
}

AmqpHandler::~AmqpHandler() {
  close_handler();
  for (int i = 0; i < m_impl->heartbeaters.size(); i++) {
    m_impl->heartbeaters[i].wait();
  }
  delete m_impl;
}
void AmqpHandler::quit() { m_impl->quit = true; }

void AmqpHandler::AmqpHandler::close_handler() {
  abnet::error_code ec;
  abnet::socket_ops::close(m_impl->sock, 0, 0, ec);
}

// Should run async
void AmqpHandler::heartbeater(AMQP::Connection *connection, uint16_t interval) {
  using clock = std::chrono::high_resolution_clock;

  clock::time_point lastSent = clock::now();
  while (m_impl->quit == false) {
    std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_RESOLUTION));
    if (connection == nullptr) {
      return;
    }
    auto currentTime = clock::now();
    std::chrono::duration<float> diff = currentTime - lastSent;
    // if time is up, then try to send heartbeat
    if (uint16_t(diff.count()) >= interval) {
      auto const lastMessage = m_impl->lastMessage.load();
      std::chrono::duration<float> writeDiff = currentTime - lastMessage;

      // If less than <interval> seconds have passed since the last message, then we skip
      if (uint16_t(writeDiff.count()) < interval) {
        lastSent = lastMessage;
      } else {
        lastSent = currentTime;
        connection->heartbeat();
      }
    }
  }
}

uint16_t AmqpHandler::onNegotiate(AMQP::Connection *connection, uint16_t interval) {

  if (m_impl->conf->heartbeat == 0) {
    return 0;
  }
  interval = interval < m_impl->conf->heartbeat ? interval : m_impl->conf->heartbeat;
  interval = interval < DEFAULT_HEARTBEAT ? DEFAULT_HEARTBEAT : interval;
  m_impl->heartbeaters.emplace_back(std::async(&AmqpHandler::heartbeater, this, connection, interval));
  return interval;
}

void AmqpHandler::onData(AMQP::Connection *connection, const char *data, size_t size) {
  std::lock_guard<std::mutex> guard(m_impl->writeMtx);
  m_impl->connection = connection;
  const size_t writen = m_impl->outBuffer.write(data, size);
  if (writen == size) {
    sendDataFromBuffer();
    m_impl->outBuffer.write(data + writen, size - writen);
  }
}

void AmqpHandler::onReady(AMQP::Connection *connection) { m_impl->connected = true; }

void AmqpHandler::onError(AMQP::Connection *connection, const char *message) { printf("AMQP error %s\n", message); }

void AmqpHandler::onClosed(AMQP::Connection *connection) {
  printf("AMQP closed connection\n");
  m_impl->quit = true;
}

bool AmqpHandler::connected() const { return m_impl->connected; }

void AmqpHandler::sendDataFromBuffer() {
  std::lock_guard<std::mutex> guard(m_impl->sendMtx);
  if (m_impl->outBuffer.available()) {
    // mesure time for heartbeat, in this case its not nessesary to be atomic
    // but read can be in critial sections
    m_impl->lastMessage.store(std::chrono::high_resolution_clock::now());
    abnet::error_code ec;
    abnet::socket_ops::send1(m_impl->sock, m_impl->outBuffer.data(), m_impl->outBuffer.available(), 0, ec);
    if (ec.value() != 0) {
      printf("Error send: %s", ec.message().c_str());
    }
    m_impl->outBuffer.drain();
  }
}