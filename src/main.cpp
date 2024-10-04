#define _CRT_SECURE_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC
#include "stdio.h"
#include "stdlib.h"
#include <thread>

#include <amqpcpp.h>

#include "amqp-handler/amqp-handler.hpp"
#include "amqp-router/amqp-router.hpp"

#include "ncnn-service/ncnn-service-realesrgan.h"

#include "mongo-client/mongo-client.hpp"
#include "image-service/image-service.hpp"

//#define STB_IMAGE_IMPLEMENTATION
//#define STBI_NO_PSD
//#define STBI_NO_TGA
//#define STBI_NO_GIF
//#define STBI_NO_HDR
//#define STBI_NO_PIC
//// #define STBI_NO_STDIO
//#include "stb-image/stb_image.h"
//#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb-image/stb_image_write.h"
//#endif

#include <crtdbg.h>

int main(int argc, char **argv) {
  //ncnn::create_gpu_instance();
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

     // Send all reports to STDOUT
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

  _CrtMemState s1, s2, s3;
  _CrtMemCheckpoint(&s1);

  //int x, y, c;
  //std::string ext = ".png";
  //std::string fullinput = "";
  //fullinput += "input";
  //unsigned char *pixeldataOne = stbi_load("cat.jpg", &x, &y, &c, 0);
  //
  //int x1, y1, c1;
  //unsigned char *pixeldataTwo = stbi_load((fullinput + "two" + ext).c_str(), &x1, &y1, &c1, 0);
  //
  //  ncnn::Net net;
  //int vdevice = ncnn::get_default_gpu_index();
  //  net.opt.use_vulkan_compute = true;
  //net.opt.use_fp16_packed = false;    // TODO:true
  //net.opt.use_fp16_storage = false;   // TODO:true
  //net.opt.use_fp16_arithmetic = false;
  //net.opt.use_int8_storage = false;   // TODO:true
  //net.opt.use_int8_arithmetic = false;
  //net.set_vulkan_device(vdevice);
  // int erz = 0;
  //{
  //  FILE *fp = fopen("../models/realesrgan-x4plus.param", "rb");
  //  if (!fp) {
  //    return -1;
  //  }
  //  erz = net.load_param(fp);
  //  fclose(fp);
  //}
  //{
  //  FILE *fp = fopen("../models/realesrgan-x4plus.bin", "rb");
  //  if (!fp) {
  //    return -1;
  //  }
  //  erz |= net.load_model(fp);
  //  fclose(fp);
  //}
  //if (erz != 0) {
  //  return -1;
  //}

  //limb::RealesrganService ns(&net, false);
  //auto ret = ns.load();
  //if (ret != liret::kOk) {
  //  printf("%s", listat::getErrorMessage(ret));
  //  return (int)ret;
  //}

  //ncnn::Mat inimageOne(x, y, (void *)pixeldataOne, (size_t)c, c);
  //ncnn::Mat outimageOne(x * 4, y * 4, (size_t)c, c);

  //  ncnn::Mat inimageTwo(x1, y1, (void *)pixeldataTwo, (size_t)c1, c1);
  //ncnn::Mat outimageTwo(x1 * 4, y1 * 4, (size_t)c1, c1);
  //ns.scale = 4;
  //ns.tilesize = 200;
  //ns.prepadding = 10;

  //auto executionLambda = [](limb::RealesrganService*ns,ncnn::Mat *inimage, ncnn::Mat *outimage) {
  //  //ns->process_matrix(*inimage, *outimage);
  //};
  //std::thread thx(executionLambda,&ns,&inimageOne,&outimageOne);
  //
  //std::thread thx1(executionLambda, &ns, &inimageTwo, &outimageTwo);
  //thx.join();
  //thx1.join();
  //stbi_write_png((fullinput + "one" + "out" + ext).c_str(), x * 4, y * 4, c, outimageOne.data, 0);
  //stbi_write_png((fullinput + "two" + "out" + ext).c_str(), x1 * 4, y1 * 4, c1, outimageTwo.data, 0);
  //delete pixeldataOne;
  //delete pixeldataTwo;

  //  ^^^^^^^^^^^^^^^
    //Initialise MongoService
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
   AmqpHandler handler("192.168.1.103", 5672);
   AMQP::Connection connection(&handler, AMQP::Login("test", "test"), "/");
   AMQP::Channel ch(&connection);
   liret ret = setRoutes(connection,ch);
   if (ret != liret::kOk) {
     printf("%s", listat::getErrorMessage(ret));
     return (int)ret;
   }
   std::cout << " [x] Awaiting RPC requests" << std::endl;
   
   //std::thread thd([&handler]() {
   //  Sleep(3000);
   //  handler.quit();
   //});

   handler.loop();
   //thd.join();

   // TODO: Implement better cleanup
   delete limb::mongoService;
   mongoc_uri_destroy(uri);
   mongoc_client_pool_destroy(pool);
   mongoc_cleanup();

   delete limb::imageService;
   ncnn::destroy_gpu_instance();
   

     _CrtMemCheckpoint(&s2);
   if (_CrtMemDifference(&s3, &s1, &s2)) {
     std::cout << "Memory leaks detected in the specific code region!\n";
     _CrtMemDumpStatistics(&s3);
   } else {
     std::cout << "No memory leaks detected in the specific code region.\n";
   }

  return 0;
}
