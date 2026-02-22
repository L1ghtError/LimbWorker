#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>

#include <amqpcpp.h>

// TODO remove ncnn dependency
#include <gpu.h>

#include "app-transport/amqp-transport.hpp"

#include "app-config.h"
#include "capabilities-provider.h"
#include "image-service/image-service.hpp"
#include "limb-app.h"
#include "media-repository/mongo-client.hpp"
#include "processor-loader.h"

#include "utils/bithacks.h"

int main(int argc, char **argv) {
  liret err = liret::kOk;

  limb::ConfigFactory configF(argc, argv);
  limb::AppConfig config;
  err = configF.getConfig(config);
  if (err != liret::kOk) {
    printf("failed to initialize config %s", listat::getErrorMessage(err));
    return EXIT_FAILURE;
  }

  bson_error_t error = {0};
  const char *uriString = config.dbConfig.uri.c_str();
  const char *dbName = config.dbConfig.dbName.c_str();
  limb::MongoClient repo(uriString, dbName);

  err = repo.init(&error);
  if (err != liret::kOk) {
    fprintf(stderr, "failed to initialize Mongo client: %s %s, %s\n", uriString, dbName, error.message);
    return EXIT_FAILURE;
  }

  limb::ImageService<limb::MongoClient &> imageService(repo);
  limb::ProcessorLoader loader({"processors"});
  limb::CapabilitiesProvider capProvider;

  limb::App<limb::ImageService<limb::MongoClient &> &, limb::ProcessorLoader &, limb::CapabilitiesProvider &>
      application(imageService, loader, capProvider);

  if (application.init() != liret::kOk) {
    fprintf(stderr, "failed to initialize application\n");
    return EXIT_FAILURE;
  }

  const int queueCount = ncnn::get_gpu_info(ncnn::get_default_gpu_index()).compute_queue_count();
  const int thdCount = std::thread::hardware_concurrency();

  limb::tp::ThreadPoolOptions options;
  options.setThreadCount(std::min(queueCount, thdCount));
  options.setQueueSize(nextPowerOfTwo(thdCount));

  limb::AmqpTransportAdapter transport(config.transportConfig, options);
  if (transport.init(&application) != liret::kOk) {
    fprintf(stderr, "failed to amqp transport\n");
    return EXIT_FAILURE;
  }

  fprintf(stdout, "[x] Awaiting RPC requests\n");

  if (transport.loop() != liret::kOk) {
    fprintf(stderr, "failed to start amqp transport loop\n");
    return EXIT_FAILURE;
  }

  // For some reason windwos SIGTERM produces thousands of memory leak
  // Seems like on windows it better aproach
  // std::thread thd([&handler]() {
  //   std::this_thread::sleep_for(std::chrono::milliseconds(200));
  //   handler.quit();
  // });
  // thd.join();

  if (ncnn::get_gpu_instance())
    ncnn::destroy_gpu_instance();

  return 0;
}
