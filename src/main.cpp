#include "stdio.h"
#include "stdlib.h"
#include <thread>

#include <amqpcpp.h>

#include "amqp-handler/amqp-handler.hpp"
#include "amqp-router/amqp-router.hpp"

#include "ncnn-service/ncnn-service-realesrgan.h"

#include "image-service/image-service.hpp"
#include "mongo-client/mongo-client.hpp"

#include "utils/bithacks.h"

// TODO: create separate integrational testing pipeline
// #define INTEGRATIONAL_TEST
#ifdef INTEGRATIONAL_TEST
#include "utils/stb-include.h"
#include <functional>
#include <future>
int test_mp() {
  limb::tp::ThreadPool threadPool;
  int x, y, c;
  std::string ext = ".png";
  std::string fullinput = "";
  fullinput += "inmem";
  unsigned char *pixeldataOne = stbi_load("inmem.png", &x, &y, &c, 0);

  int x1, y1, c1;
  unsigned char *pixeldataTwo = stbi_load((fullinput + ext).c_str(), &x1, &y1, &c1, 0);

  limb::NcnnService *firstServ = nullptr;
  auto ret = limb::imageService->imageProcessorNew((IP_IMAGE_REALESRGAN), &firstServ);
  if (ret != liret::kOk) {
    printf("first %s", listat::getErrorMessage(ret));
    return (int)ret;
  }

  limb::NcnnService *secondServ = nullptr;
  ret = limb::imageService->imageProcessorNew((IP_IMAGE_REALESRGAN), &secondServ);
  if (ret != liret::kOk) {
    printf("first %s", listat::getErrorMessage(ret));
    return (int)ret;
  }

  ncnn::Mat inimageOne(x, y, (void *)pixeldataOne, (size_t)c, c);
  ncnn::Mat outimageOne(x * 4, y * 4, (size_t)c, c);

  ncnn::Mat inimageTwo(x1, y1, (void *)pixeldataTwo, (size_t)c1, c1);
  ncnn::Mat outimageTwo(x1 * 4, y1 * 4, (size_t)c1, c1);

  auto executionLambda = [](limb::NcnnService *ns, ncnn::Mat *inimage, ncnn::Mat *outimage) {
    ns->process_matrix(*inimage, *outimage);
  };

  auto lamOne = [executionLambda, firstServ, &inimageOne, &outimageOne]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(900));
    executionLambda(firstServ, &inimageOne, &outimageOne);
  };
  auto lamTwo = [executionLambda, secondServ, &inimageTwo, &outimageTwo]() {
    executionLambda(secondServ, &inimageTwo, &outimageTwo);
  };
  std::packaged_task<void()> packOne(lamOne);
  std::packaged_task<void()> packTwo(lamTwo);
  std::future<void> futOne = packOne.get_future();
  std::future<void> futTwo = packTwo.get_future();
  threadPool.post(std::move(packOne));
  threadPool.post(std::move(packTwo));
  futOne.get();
  futTwo.get();
  stbi_write_png((fullinput + "one" + "out" + ext).c_str(), x * 4, y * 4, c, outimageOne.data, 0);
  stbi_write_png((fullinput + "two" + "out" + ext).c_str(), x1 * 4, y1 * 4, c1, outimageTwo.data, 0);

  delete pixeldataOne;
  delete pixeldataTwo;
  return 0;
  //  ^^^^^^^^^^^^^^^
}
#endif
int main(int argc, char **argv) {
  mongoc_init();
  ncnn::create_gpu_instance();

  bson_error_t error = {0};
  const char *uri_string = "mongodb://192.168.31.79:8005/?appname=client-example";
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
  liret err = limb::mongoService->ping(&error);
  if (err != liret::kOk) {
    fprintf(stderr,
            "failed to ping: %s\n"
            "error message:  %s\n",
            uri_string, error.message);
  }
  // Initialise ImageSerice
  limb::imageService = new limb::ImageService;
  limb::ImageServiceOptions opts;
  opts.paramfullpath = "../models/realesrgan-x4plus.param";
  opts.modelfullpath = "../models/realesrgan-x4plus.bin";
  opts.scale = 4;
  opts.tilesize = 200;
  opts.prepadding = 10;
  opts.tta_mode = false;
  opts.vulkan_device_index = ncnn::get_default_gpu_index();
  opts.type = IP_IMAGE_REALESRGAN;
  limb::imageService->addOption(opts);

// Tests
#ifdef INTEGRATIONAL_TEST
  return test_mp();
#endif
  // AMQP preprocess
  AmqpConfig amqpConf;
  amqpConf.heartbeat = 60;
  AmqpHandler handler("192.168.31.103", 5672, &amqpConf);
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
  liret ret = setRoutes(tp, connection, ch);
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

  // TODO: Implement better cleanup
  delete limb::mongoService;
  mongoc_uri_destroy(uri);
  mongoc_client_pool_destroy(pool);
  mongoc_cleanup();

  delete limb::imageService;
  ncnn::destroy_gpu_instance();

  return 0;
}
