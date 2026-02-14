#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>

#include <amqpcpp.h>
#include <gpu.h>

#include "utils/stb-wrap.h"

#include "amqp-handler/amqp-handler.hpp"
#include "amqp-router/amqp-router.hpp"

#include "capabilities-provider.h"
#include "image-service/image-service.hpp"
#include "limb-app.h"
#include "media-repository/mongo-client.hpp"
#include "processor-loader.h"

#include "utils/bithacks.h"
#include "utils/image-info.h"

// TODO: create separate integrational testing pipeline
// #define INTEGRATIONAL_TEST
#ifdef INTEGRATIONAL_TEST
#include <functional>
#include <future>

template <class limbApp> size_t findRealEsrganIndex(limbApp &application) {
  const std::string name("Real-ESRGAN");
  const size_t pc = application.m_processorLoader.processorCount();

  for (size_t i = 0; i < pc; i++) {
    if (application.m_processorLoader.processorName(i) == name) {
      return i;
    }
  }
  return -1;
}

template <class limbApp> int test_mp(limbApp &application) {
  limb::tp::ThreadPool threadPool;
  int x, y, c;
  std::string ext = ".png";
  std::string fullinput = "";
  fullinput += "input";
  unsigned char *pixeldataOne = stbi_load((fullinput + ext).c_str(), &x, &y, &c, 0);

  int x1, y1, c1;
  unsigned char *pixeldataTwo = stbi_load((fullinput + ext).c_str(), &x1, &y1, &c1, 0);
  const size_t realesrganIndex = findRealEsrganIndex(application);
  if (realesrganIndex == -1) {
    return -1;
  }

  limb::ImageProcessor *firstServ = nullptr;
  auto ret = application.m_mediaService.getProcessor(realesrganIndex, &firstServ);
  if (ret != liret::kOk) {
    printf("First processor %s", listat::getErrorMessage(ret));
    return (int)ret;
  }

  limb::ImageProcessor *secondServ = nullptr;
  ret = application.m_mediaService.getProcessor(realesrganIndex, &secondServ);
  if (ret != liret::kOk) {
    printf("Second processor %s", listat::getErrorMessage(ret));
    return (int)ret;
  }

  limb::ImageInfo inimageOne = {.data = pixeldataOne, .w = x, .h = y, .c = c};
  limb::ImageInfo outimageOne;

  limb::ImageInfo inimageTwo = {.data = pixeldataTwo, .w = x1, .h = y1, .c = c1};
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
  // threadPool.post(std::move(packTwo));
  futOne.get();
  // futTwo.get();
  lib_image_write_png((fullinput + "one" + "out" + ext).c_str(), x * 4, y * 4, c, outimageOne.data, 0);
  // lib_image_write_png((fullinput + "two" + "out" + ext).c_str(), x1 * 4, y1 * 4, c1, outimageTwo.data, 0);

  delete pixeldataOne;
  delete pixeldataTwo;
  return 0;
  //  ^^^^^^^^^^^^^^^
}
#endif
int main(int argc, char **argv) {
  liret err = liret::kOk;

  bson_error_t error = {0};
  const char *uriString = "mongodb://192.168.31.33:8005/?appname=client-example";
  const char *dbString = "LimbDB";
  limb::MongoClient repo(uriString, dbString);
#ifndef INTEGRATIONAL_TEST
  err = repo.init(&error);
  if (err != liret::kOk) {
    fprintf(stderr, "failed to initialize Mongo client: %s %s, %s\n", uriString, dbString, error.message);
    return EXIT_FAILURE;
  }
#endif

  limb::ImageService<limb::MongoClient &> imageService(repo);
  limb::ProcessorLoader loader({"processors"});
  limb::CapabilitiesProvider capProvider;

  limb::App<limb::ImageService<limb::MongoClient &> &, limb::ProcessorLoader &, limb::CapabilitiesProvider &>
      application(imageService, loader, capProvider);
  application.init();

// Tests
#ifdef INTEGRATIONAL_TEST
  int test_ret = test_mp(application);

  if (ncnn::get_gpu_instance())
    ncnn::destroy_gpu_instance();
  return test_ret;
#endif

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
