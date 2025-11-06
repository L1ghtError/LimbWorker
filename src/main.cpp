#include "stdio.h"
#include "stdlib.h"
#include <thread>

#include <amqpcpp.h>
#include <dlfcn.h>
#include <gpu.h>

#include "amqp-handler/amqp-handler.hpp"
#include "amqp-router/amqp-router.hpp"

#include "image-service/image-service.hpp"
#include "mongo-client/mongo-client.hpp"

#include "utils/bithacks.h"
#include "utils/image-info.h"

// TODO: create separate integrational testing pipeline
#define INTEGRATIONAL_TEST
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
  unsigned char *pixeldataOne = stbi_load((fullinput + ext).c_str(), &x, &y, &c, 0);

  int x1, y1, c1;
  unsigned char *pixeldataTwo = stbi_load((fullinput + ext).c_str(), &x1, &y1, &c1, 0);

  limb::ImageProcessor *firstServ = nullptr;
  auto ret = limb::imageService->getProcessor(IP_IMAGE_REALESRGAN, &firstServ);
  if (ret != liret::kOk) {
    printf("first %s", listat::getErrorMessage(ret));
    return (int)ret;
  }

  limb::ImageProcessor *secondServ = nullptr;
  ret = limb::imageService->getProcessor(IP_IMAGE_REALESRGAN, &secondServ);
  if (ret != liret::kOk) {
    printf("first %s", listat::getErrorMessage(ret));
    return (int)ret;
  }

  limb::ImageInfo inimageOne = {.data = pixeldataOne, .w = (uint32_t)x, .h = (uint32_t)y, .c = (uint32_t)c};
  limb::ImageInfo outimageOne;

  limb::ImageInfo inimageTwo = {.data = pixeldataTwo, .w = (uint32_t)x1, .h = (uint32_t)y1, .c = (uint32_t)c1};
  limb::ImageInfo outimageTwo;

  auto executionLambda = [](limb::ImageProcessor *ns, limb::ImageInfo *inimage, limb::ImageInfo *outimage) {
    ns->process_image(*inimage, *outimage);
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
  liret err = liret::kOk;
#ifndef INTEGRATIONAL_TEST
  mongoc_init();

  bson_error_t error = {0};
  const char *uri_string = "mongodb://192.168.88.245:8005/?appname=client-example";
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
  err = limb::mongoService->ping(&error);
  if (err != liret::kOk) {
    fprintf(stderr,
            "failed to ping: %s\n"
            "error message:  %s\n",
            uri_string, error.message);
    return EXIT_FAILURE;
  }
#endif

  // Initialise ImageService
  limb::imageService = new limb::ImageService;

  limb::ImageProcessor *realesrgan_ptr = nullptr;
  const char *libpath = "./processors/realesrgan/librealesrgan.so";
  void *handle = dlopen(libpath, RTLD_NOW | RTLD_LOCAL);
  typedef limb::ImageProcessor *(*create_fn_t)();
  typedef void (*destroy_fn_t)(limb::ImageProcessor *);
  create_fn_t create_fn = nullptr;
  destroy_fn_t destroy_fn = nullptr;
  if (handle) {

    create_fn = reinterpret_cast<create_fn_t>(dlsym(handle, "createProcessor"));
    destroy_fn = reinterpret_cast<destroy_fn_t>(dlsym(handle, "destroyProcessor"));
    if (create_fn && destroy_fn) {
      realesrgan_ptr = create_fn();
    }
  } else {
    fprintf(stderr, "warning: failed to open shared library '%s': %s\n", libpath, dlerror());
    return EXIT_FAILURE;
  }

  realesrgan_ptr->init();
  realesrgan_ptr->load();
  err = limb::imageService->addProcessor(IP_IMAGE_REALESRGAN, reinterpret_cast<limb::ImageProcessor *>(realesrgan_ptr));
  if (err != liret::kOk) {
    fprintf(stderr, "failed to add realesrgan processor\n");
  }

// Tests
#ifdef INTEGRATIONAL_TEST
  int test_ret = test_mp();
  destroy_fn(realesrgan_ptr);
  dlclose(handle);
  if (ncnn::get_gpu_instance)
    ncnn::destroy_gpu_instance();
  return test_ret;
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
#ifndef INTEGRATIONAL_TEST
  // TODO: Implement better cleanup
  delete limb::mongoService;
  mongoc_uri_destroy(uri);
  mongoc_client_pool_destroy(pool);
  mongoc_cleanup();
#endif

  delete limb::imageService;

  destroy_fn(realesrgan_ptr);
  dlclose(handle);
  if (ncnn::get_gpu_instance)
    ncnn::destroy_gpu_instance();

  return 0;
}
