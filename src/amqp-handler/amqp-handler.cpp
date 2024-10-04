#include "amqp-handler/amqp-handler.hpp"

#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)) && !defined(__CYGWIN__)
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#define CONNECTION_TIMEOUT 5000 // 5 seconds

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
  AmqpHandlerImpl()
      : connected(false), connection(nullptr), quit(false), keepAlive(true), inputBuffer(AmqpHandler::BUFFER_SIZE),
        outBuffer(AmqpHandler::BUFFER_SIZE), tmpBuff(AmqpHandler::TEMP_BUFFER_SIZE), sock(INVALID_SOCKET) {
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
  }

  BOOL keepAlive = TRUE;
  WSADATA wsaData;
  fd_set readfds, writefds;
  struct timeval timeout;

  Buffer inputBuffer;
  Buffer outBuffer;
  SOCKET sock;
  AMQP::Connection *connection;
  std::vector<char> tmpBuff;
  bool quit;
  bool connected;
};

AmqpHandler::AmqpHandler(const char *host, uint16_t port) : m_impl(new AmqpHandlerImpl) {
  const int timeout = CONNECTION_TIMEOUT;
  do {
    int err = WSAStartup(MAKEWORD(2, 2), &m_impl->wsaData);
    if (err != 0) {
      break;
    }
    // Sock creation
    sockaddr_in service;
    service.sin_family = AF_INET;
    if (inet_pton(AF_INET, host, &service.sin_addr.s_addr) != 1) {
      break;
    }
    service.sin_port = htons(port);

    m_impl->sock = socket(service.sin_family, SOCK_STREAM, IPPROTO_TCP);
    if (m_impl->sock == INVALID_SOCKET) {
      break;
    }

    // Set receive timeout (SO_RCVTIMEO) to 5 seconds
    if (setsockopt(m_impl->sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0) {
      break;
    }

    // Set send timeout (SO_SNDTIMEO) to 5 seconds (optional)
    if (setsockopt(m_impl->sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout)) < 0) {
      break;
    }

    err = setsockopt(m_impl->sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&m_impl->keepAlive, sizeof(m_impl->keepAlive));
    if (err == SOCKET_ERROR) {
      break;
    }

    err = connect(m_impl->sock, (SOCKADDR *)&service, sizeof(service));
    if (err == SOCKET_ERROR) {
      err = WSAGetLastError();
      break;
    }
    return;
  } while (0);
  if (m_impl->sock != INVALID_SOCKET) {
    closesocket(m_impl->sock);
  }
  m_impl->sock = INVALID_SOCKET;
}

void AmqpHandler::loop() {
  int err = 0;
  while (!m_impl->quit) {
    FD_ZERO(&m_impl->readfds);
    FD_ZERO(&m_impl->writefds);

    FD_SET(m_impl->sock, &m_impl->readfds);
    FD_SET(m_impl->sock, &m_impl->writefds);

    err = select(0, &m_impl->readfds, &m_impl->writefds, NULL, &m_impl->timeout);

    if (err == SOCKET_ERROR) {
      printf("Select error: %d\n", WSAGetLastError());
      break;
    } else if (err == 0) {
      // Timeout
      // printf("Timeout occurred. No data received or sent.\n");
    } else {
      // Check if data is available for reading
      if (FD_ISSET(m_impl->sock, &m_impl->readfds)) {
        unsigned long bytesAvailable = 0;
        ioctlsocket(m_impl->sock, FIONREAD, &bytesAvailable);

        if (m_impl->tmpBuff.size() < bytesAvailable) {
          m_impl->tmpBuff.resize(bytesAvailable, 0);
        }

        int bytesRead = recv(m_impl->sock, &m_impl->tmpBuff[0], bytesAvailable, 0);
        m_impl->inputBuffer.write(m_impl->tmpBuff.data(), bytesAvailable);
      }

      if (m_impl->connection && m_impl->inputBuffer.available()) {
        size_t count = m_impl->connection->parse(m_impl->inputBuffer.data(), m_impl->inputBuffer.available());

        if (count == m_impl->inputBuffer.available()) {
          m_impl->inputBuffer.drain();
        } else if (count > 0) {
          m_impl->inputBuffer.shl(count);
        }
      }
      //sendDataFromBuffer();
    }
  }
  if (m_impl->quit == 0) {
    printf("Network loop force quit!\n");
  }
  if (m_impl->quit && m_impl->outBuffer.available()) {
    sendDataFromBuffer();
  }
}

AmqpHandler::~AmqpHandler() {
  close();
  delete m_impl;
}

void AmqpHandler::quit() { m_impl->quit = true; }

void AmqpHandler::AmqpHandler::close() {

  closesocket(m_impl->sock);
  WSACleanup();
}

void AmqpHandler::onData(AMQP::Connection *connection, const char *data, size_t size) {
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
  if (m_impl->outBuffer.available()) {
    send(m_impl->sock, m_impl->outBuffer.data(), (int)m_impl->outBuffer.available(), 0);
    m_impl->outBuffer.drain();
  }
}