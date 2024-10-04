#include "amqp-router/amqp-router.hpp"

#include "mongo-client/mongo-client.hpp"

#include "image-service/data-models.hpp"
#include "image-service/image-service.hpp"
#include <chrono>

char *extractBody(const AMQP::Message &message) {
  const int bodySize = message.bodySize();
  char *msg = new char[bodySize];
  const char *body = message.body();
  memcpy(msg, body, bodySize);
  return msg;
}

liret setRoutes(AMQP::Connection &conn, AMQP::Channel &ch) {
  ch.setQos(1);
  if (!ch.usable()) {
    return liret::kInvalidInput;
  }
  // ping queue
  ch.declareQueue("ping_queue");
  ch.consume("").onReceived([&ch](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
    std::cout << " [.] ping body(" << message.body() << ")" << " id:" << message.correlationID() << std::endl;
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

  ch.consume("")
      .onReceived([&ch, strUpscaleImage](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        const char *body = extractBody(message);
        limb::MUpscaleImage pbody;
        pbody.berawtoh(body);

        printf("Got connectoin %s\n", message.correlationID().c_str());
        // Progress response
        int iteration = 0;
        std::chrono::high_resolution_clock::time_point firstEnt;
        auto progressCb = [&message, &iteration, &firstEnt, &ch, deliveryTag](float progress) {
          if (iteration == 0) {
            firstEnt = std::chrono::high_resolution_clock::now();
          } else if (iteration == 2)
          {
            auto currentEnt = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float, std::milli> duration = currentEnt - firstEnt;
            char prog[32] = {0};
            const float dur = duration.count();
            float estimation = (dur / progress) - dur;
            snprintf(prog, sizeof(prog), "%.2f:%.2fms", progress, estimation);
            printf("Res-%s / %.2fms \n", prog, duration.count());
          AMQP::Envelope env(prog);
          const std::string corId = message.correlationID();
          const std::string repl = message.replyTo();
          env.setCorrelationID(corId);
          ch.publish("", repl, env);
          }
   
          ++iteration;
        };

        
          liret ret = limb::imageService->handleUpscaleImage(pbody, progressCb);
        if (ret != liret::kOk) {
            printf("Err upscale %s", listat::getErrorMessage(ret));
        }
        char end[] = "end";
        AMQP::Envelope env(end);

        const std::string corId = message.correlationID();
        const std::string repl = message.replyTo();
        env.setCorrelationID(corId);
        ch.publish("", repl, env);
        auto currentEnt = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = currentEnt - firstEnt;
        printf("sent %s / %.2fms \n", end, duration.count());
        ch.ack(deliveryTag);
        delete body;
      });

  return liret::kOk;
}