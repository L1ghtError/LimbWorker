#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>

#include <amqpcpp.h>

// TODO remove ncnn dependency
#include <gpu.h>

#include "amqp-handler/amqp-handler.hpp"
#include "amqp-router/amqp-router.hpp"

#include "capabilities-provider.h"
#include "image-service/image-service.hpp"
#include "limb-app.h"
#include "media-repository/mongo-client.hpp"
#include "processor-loader.h"

#include "utils/bithacks.h"

int main(int argc, char **argv) {
  liret err = liret::kOk;

  bson_error_t error = {0};
  const char *uriString = "mongodb://192.168.31.33:8005/?appname=client-example";
  const char *dbString = "LimbDB";
  limb::MongoClient repo(uriString, dbString);

  err = repo.init(&error);
  if (err != liret::kOk) {
    fprintf(stderr, "failed to initialize Mongo client: %s %s, %s\n", uriString, dbString, error.message);
    return EXIT_FAILURE;
  }

  limb::ImageService<limb::MongoClient &> imageService(repo);
  limb::ProcessorLoader loader({"processors"});
  limb::CapabilitiesProvider capProvider;

  limb::App<limb::ImageService<limb::MongoClient &> &, limb::ProcessorLoader &, limb::CapabilitiesProvider &>
      application(imageService, loader, capProvider);
  application.init();

  // AMQP preprocess
  AmqpConfig amqpConf;
  amqpConf.heartbeat = 60;
  AmqpHandler handler("192.168.88.250", 5672, &amqpConf);
  AMQP::Connection connection(&handler, AMQP::Login("test", "test"), "/");
  AMQP::Channel ch(&connection);
  // Thread pool preprocess
  uint32_t thdCount = ncnn::get_gpu_info(ncnn::get_default_gpu_index()).compute_queue_count();
  // TODO: set right maximum available thread count with chunk size
  thdCount = thdCount > 3 ? 3 : thdCount;
  limb::tp::ThreadPoolOptions tpOptions;

  tpOptions.setThreadCount(thdCount);
  tpOptions.setQueueSize(nextPowerOfTwo(thdCount));
  limb::tp::ThreadPool tp(tpOptions);
  // Initialise routes
  ch.setQos(thdCount);
  liret ret = setRoutes(&application, tp, connection, ch);
  if (ret != liret::kOk) {
    printf("%s", listat::getErrorMessage(ret));
    return (int)ret;
  }
  std::cout << " [x] Awaiting RPC requests" << std::endl;

  // For some reason windwos SIGTERM produces thousands of memory leak
  // Seems like on windows it better aproach
  // std::thread thd([&handler]() {
  //   std::this_thread::sleep_for(std::chrono::milliseconds(200));
  //   handler.quit();
  // });

  handler.loop();
  // thd.join();
  if (ncnn::get_gpu_instance())
    ncnn::destroy_gpu_instance();

  return 0;
}
