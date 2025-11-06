#include "amqp-router/amqp-router.hpp"

#include "mongo-client/mongo-client.hpp"

#include "image-service/image-service.hpp"
#include "utils/estimate.h"
#include <future>
char *extractBody(const AMQP::Message &message) {
  const int bodySize = message.bodySize();
  char *msg = new char[bodySize];
  const char *body = message.body();
  memcpy(msg, body, bodySize);
  return msg;
}

liret setRoutes(limb::tp::ThreadPool &tp, AMQP::Connection &conn, AMQP::Channel &ch) {
  if (!ch.usable()) {
    return liret::kInvalidInput;
  }
  // ping queue
  ch.declareQueue("ping_queue");
  ch.consume("").onReceived([&ch](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
    std::cout << " [.] ping body(" << message.body() << ")"
              << " id:" << message.correlationID() << std::endl;
    char *recvMsg = (char *)malloc((28 + message.bodySize()) * sizeof(*message.body()));
    std::string msg = "Hello, I can works with you";
    AMQP::Envelope env(msg.c_str());
    std::string repl = message.replyTo();
    env.setCorrelationID(message.correlationID());
    ch.publish("", repl, env);
    ch.ack(deliveryTag);
    free(recvMsg);
  });
  // test queues
  const char debugSeveralMsgs[] = "DebugSeveralMsgs";
  ch.declareQueue(debugSeveralMsgs);
  ch.consume("").onReceived([&ch](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
    printf(" [.] DebugSeveralMsgs body(%s) id:%s\n", message.body(), message.correlationID().c_str());
    char *recvMsg = (char *)malloc((message.bodySize()) * sizeof(*message.body()));
    std::string repl = message.replyTo();

    std::string msg = "Hello, I can works with you\r\n";
    AMQP::Envelope envOne(msg.c_str());
    envOne.setCorrelationID(message.correlationID());
    ch.publish("", repl, envOne);
    printf("\t--first %s\n", msg.c_str());

    AMQP::Envelope envTwo(recvMsg);
    envTwo.setCorrelationID(message.correlationID());
    ch.publish("", repl, envTwo);
    printf("\t --second %s\n", recvMsg);

    char end[] = "end";
    AMQP::Envelope envThree(end);
    envThree.setCorrelationID(recvMsg);
    ch.publish("", repl, envThree);
    ch.ack(deliveryTag);
    printf("\t--end\n");
    free(recvMsg);
  });
  // UpscaleImage
  const char strUpscaleImage[] = "UpscaleImage";
  ch.declareQueue(strUpscaleImage);

  ch.consume("").onReceived([&ch, &tp](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
    const char *body = extractBody(message);
    limb::MUpscaleImage pbody;
    pbody.berawtoh(body);
    printf("Got connectoin %s|%s\n", message.correlationID().c_str(), message.replyTo().c_str());

    std::string correlationID = message.correlationID();
    std::string replyTo = message.replyTo();
    auto handler = [correlationID, replyTo, deliveryTag, body, pbody, &ch]() {
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

      liret ret = limb::imageService->processImage(pbody, progressCb);
      if (ret != liret::kOk) {
        printf("Err upscale %s\n", listat::getErrorMessage(ret));
      }

      AMQP::Envelope env(END_MESSAGE);
      env.setCorrelationID(correlationID);

      ch.publish("", replyTo, env);
      auto currentEnt = std::chrono::high_resolution_clock::now();
      std::chrono::duration<float, std::milli> duration = currentEnt - firstEnt;
      printf("sent %s / %.2fms \n", END_MESSAGE, duration.count());
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