#include "stdio.h"
#include "stdlib.h"
#include <thread>

#include <amqpcpp.h>

#include "amqp-handler/amqp-handler.hpp"
#include "amqp-router/amqp-router.hpp"

#include "ncnn-service/ncnn-service-realesrgan.h"

#include "image-service/image-service.hpp"
#include "mongo-client/mongo-client.hpp"

int main(int argc, char **argv) {

  mongoc_init();
  ncnn::create_gpu_instance();

  bson_error_t error = {0};
  const char *uri_string = "mongodb://192.168.1.100:8005/?appname=client-example";
  mongoc_uri_t *uri;
  uri = mongoc_uri_new_with_error(uri_string, &error);

  if (!uri) {
    fprintf(stderr,
            "failed to parse URI: %s\n"
            "error message:       %s\n",
            uri_string, error.message);
    return EXIT_FAILURE;
  }

  mongoc_client_pool_t *pool = mongoc_client_pool_new(uri);
  if (!pool) {
    return EXIT_FAILURE;
  }
  limb::mongoService = new limb::MongoService("LimbDB", pool);

  // Initialise ImageSerice
  limb::imageService = new limb::ImageService;
  limb::ImageServiceOptions opts;
  opts.paramfullpath = "../models/realesrgan-x4plus.param";
  opts.modelfullpath = "../models/realesrgan-x4plus.bin";
  opts.scale = 4;
  opts.tilesize = 400;
  opts.prepadding = 10;
  opts.tta_mode = false;
  opts.vulkan_device_index = ncnn::get_default_gpu_index();
  opts.type = IP_IMAGE_REALESRGAN;
  limb::imageService->addOption(opts);

  // Rabbitmq
  AmqpConfig amqpConf;
  amqpConf.heartbeat = 60;
  AmqpHandler handler("192.168.1.103", 5672, &amqpConf);
  AMQP::Connection connection(&handler, AMQP::Login("test", "test"), "/");
  AMQP::Channel ch(&connection);
  liret ret = setRoutes(connection, ch);
  if (ret != liret::kOk) {
    printf("%s", listat::getErrorMessage(ret));
    return (int)ret;
  }
  std::cout << " [x] Awaiting RPC requests" << std::endl;

  // For some reason windwos SIGTERM produces thousands of memory leak
  // Seems like on windows it better aproach
  // std::thread thd([&handler]() {
  //   Sleep(3000);
  //   handler.quit();
  // });

  handler.loop();
  // thd.join();

  // TODO: Implement better cleanup
  delete limb::mongoService;
  mongoc_uri_destroy(uri);
  mongoc_client_pool_destroy(pool);
  mongoc_cleanup();

  delete limb::imageService;
  ncnn::destroy_gpu_instance();

  return 0;
}
