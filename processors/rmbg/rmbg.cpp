#include "rmbg/rmbg.h"

#include "cpu.h"
#include <gpu.h>

#include <algorithm>
#include <fstream>
#include <mutex>

static const uint32_t rmbg_postproc_spv_data[] = {
#include "rmbg_postproc.spv.hex.h"
};

constexpr const char *kModelPath = "../models/RMBG-1.4/onnx/model.onnx";

constexpr int kTargetHeight = 1024;
constexpr int kTargetWidth = 1024;
constexpr auto cudaEPName = "CUDAExecutionProvider";

extern "C" LIMB_API limb::RmbgModule *GetProcessorModule() { return new limb::RmbgModule; }

namespace {
inline liret loadFile(const char *path, std::vector<uint8_t> &buffer) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    return liret::kNotFound;

  std::streampos pos = file.tellg();
  if (pos == std::streampos(-1))
    return liret::kAborted;

  std::size_t size = static_cast<std::size_t>(pos);

  if (size == 0) {
    buffer.clear();
    return liret::kOk;
  }

  file.seekg(0, std::ios::beg);

  buffer.resize(size);
  file.read((char *)buffer.data(), size);
  return liret::kOk;
}

inline void preprocessImage(const unsigned char *pixels, int type, int w, int h, int target_w, int target_h,
                            ncnn::Mat &out) {

  out = ncnn::Mat::from_pixels_resize(pixels, type, w, h, target_w, target_h);

  float mean_vals[3] = {127.5f, 127.5f, 127.5f};
  const float norm_vals[3] = {1 / 255.0f, 1 / 255.0f, 1 / 255.0f};
  out.substract_mean_normalize(mean_vals, norm_vals);
}

bool cudaSuppored() {
  auto providers = Ort::GetAvailableProviders();

  return std::any_of(providers.begin(), providers.end(), [](const std::string &p) { return p == cudaEPName; });
}

} // namespace

namespace limb {
RmbgContainer::RmbgContainer()
    : env(ORT_LOGGING_LEVEL_WARNING, g_processorName),
      memInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU)) {}

RmbgContainer::~RmbgContainer() {
  if (session) {
    delete session;
    session = nullptr;
  }
}

liret RmbgContainer::init() {
  // The NCNN GPU instance is unique per shared library, so each module needs to handle it manually.
  ncnn::create_gpu_instance();

  sessionOptions.SetLogSeverityLevel(ORT_LOGGING_LEVEL_WARNING);

  if (cudaSuppored()) {
    memset(&cudaOptions, 0, sizeof(cudaOptions));
    cudaOptions.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
    cudaOptions.gpu_mem_limit = SIZE_MAX;

    sessionOptions.AppendExecutionProvider_CUDA(cudaOptions);
    sessionOptions.DisableMemPattern();
  }

  sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

  if (loadFile(kModelPath, modelBuffer) != liret::kOk) {
    deinit();
    return liret::kNotFound;
  }

  session = new Ort::Session(env, modelBuffer.data(), modelBuffer.size(), sessionOptions);

  return liret::kOk;
}

liret RmbgContainer::deinit() {
  if (session) {
    delete session;
    session = nullptr;
  }
  modelBuffer.clear();

  sessionOptions = Ort::SessionOptions();

  // Since this module is a separate shared library, it is safe to destroy the NCNN GPU instance,
  // and it will not affect other modules.
  ncnn::destroy_gpu_instance();
  return liret::kOk;
}

ImageProcessor *RmbgContainer::tryAcquireProcessor() {
  const int gpuIndex = ncnn::get_default_gpu_index();

  if (!session) {
    return nullptr;
  }

  return new RmbgProcessor(memInfo, session, gpuIndex);
}

void RmbgContainer::reclaimProcessor(ImageProcessor *proc) { delete proc; }

RmbgProcessor::RmbgProcessor(Ort::MemoryInfo &memInfo, Ort::Session *session, int gpuIndex)
    : memInfo(memInfo), session(session), gpuIndex(gpuIndex) {
  std::vector<ncnn::vk_specialization_type> specializations(0);

  vulkanDevice = new ncnn::VulkanDevice(gpuIndex);

  rmbg_postproc = new ncnn::Pipeline(vulkanDevice);
  rmbg_postproc->set_optimal_local_size_xyz(32, 32, 3);
  rmbg_postproc->create(rmbg_postproc_spv_data, sizeof(rmbg_postproc_spv_data), specializations);
}

RmbgProcessor::~RmbgProcessor() {
  if (vulkanDevice) {
    delete vulkanDevice;
    vulkanDevice = nullptr;
  }

  if (rmbg_postproc) {
    delete rmbg_postproc;
    rmbg_postproc = nullptr;
  }
};

liret RmbgProcessor::process_image(const ImageInfo &inimage, ImageInfo &outimage, const ProgressCallback &procb) const {
  outimage.data = nullptr;
  outimage.w = inimage.w;
  outimage.h = inimage.h;
  outimage.c = 4; // Always output RGBA

  const size_t datasize = outimage.w * outimage.h * outimage.c;

  const unsigned char *pixeldata = (const unsigned char *)inimage.data;
  const int w = inimage.w;
  const int h = inimage.h;

  auto blobDeleter = [this](ncnn::VkAllocator *ptr) { vulkanDevice->reclaim_blob_allocator(ptr); };
  auto stagingDeleter = [this](ncnn::VkAllocator *ptr) { vulkanDevice->reclaim_staging_allocator(ptr); };

  std::unique_ptr<ncnn::VkAllocator, decltype(blobDeleter)> blob_vkallocator(vulkanDevice->acquire_blob_allocator(),
                                                                             blobDeleter);

  std::unique_ptr<ncnn::VkAllocator, decltype(stagingDeleter)> staging_vkallocator(
      vulkanDevice->acquire_staging_allocator(), stagingDeleter);

  ncnn::Option opt;
  opt.use_vulkan_compute = true;
  opt.use_fp16_packed = true;
  opt.use_fp16_storage = true;
  opt.use_fp16_arithmetic = false;
  opt.use_int8_storage = true;
  opt.use_int8_arithmetic = false;
  opt.blob_vkallocator = blob_vkallocator.get();
  opt.staging_vkallocator = staging_vkallocator.get();

  // If device support fp16 then it will be used instead of regular float
  const size_t in_out_tile_elemsize = opt.use_fp16_storage ? 2u : 4u;

  // Inform Progress callback about start
  procb(0.0);

  ncnn::Mat in;

  // Used in shared program
  if (opt.use_fp16_storage && opt.use_int8_storage) {
    in = ncnn::Mat(w, h, (unsigned char *)pixeldata, (size_t)inimage.c, 1);
  } else {
    // TODO: implement support for float values
  }

  // preproc
  ncnn::Mat pImage;
  int type = inimage.c == 4 ? ncnn::Mat::PIXEL_RGBA2RGB : ncnn::Mat::PIXEL_RGB;
  preprocessImage((const unsigned char *)in.data, type, w, h, kTargetWidth, kTargetHeight, pImage);

  std::array<int64_t, 4> inDims = {1, 3, pImage.w, pImage.h};
  std::array<int64_t, 4> outDims = {1, 1, pImage.w, pImage.h};

  auto inputTensor =
      Ort::Value::CreateTensor<float>(memInfo, (float *)pImage.data, pImage.total(), inDims.data(), inDims.size());
  Ort::Value outputTensor;

  try {
    std::vector<std::string> providers = Ort::GetAvailableProviders();

    const char *input_names[] = {"input"};
    const char *output_names[] = {"output"};

    session->Run(runOptions, input_names, &inputTensor, 1, output_names, &outputTensor, 1);

  } catch (const std::exception &e) {
    return liret::kAborted;
  }

  ncnn::Mat mask(pImage.w, pImage.h, outputTensor.GetTensorMutableData<float>(), 4, 1);

  ncnn::VkCompute cmd(vulkanDevice);

  ncnn::VkMat in_gpu;
  ncnn::VkMat mask_gpu;
  ncnn::VkMat out_gpu;
  if (opt.use_fp16_storage && opt.use_int8_storage) {
    out_gpu.create(outimage.w, outimage.h, (size_t)outimage.c, 1, blob_vkallocator.get());
  } else {
    // TODO: implement support for float values
  }

  cmd.record_clone(mask, mask_gpu, opt);
  cmd.submit_and_wait();
  cmd.reset();

  cmd.record_clone(in, in_gpu, opt);
  cmd.submit_and_wait();
  cmd.reset();

  // postproc
  {
    std::vector<ncnn::VkMat> bindings(3);
    bindings[0] = in_gpu;
    bindings[1] = mask_gpu;
    bindings[2] = out_gpu;

    std::vector<ncnn::vk_constant_type> constants(7);
    constants[0].i = outimage.w;
    constants[1].i = outimage.h;
    constants[2].i = in.cstep;
    constants[3].i = mask_gpu.w;
    constants[4].i = mask_gpu.h;
    constants[5].i = inimage.c;
    constants[6].i = outimage.c;

    ncnn::VkMat dispatcher;
    dispatcher.w = outimage.w;
    dispatcher.h = outimage.h;
    dispatcher.c = outimage.c;

    cmd.record_pipeline(rmbg_postproc, bindings, constants, dispatcher);
  }

  outimage.size = datasize;
  outimage.data = new unsigned char[datasize];
  ncnn::Mat outmat = ncnn::Mat(w, h, (void *)outimage.data, (size_t)outimage.c);

  cmd.record_clone(out_gpu, outmat, opt);
  cmd.submit_and_wait();

  return liret::kOk;
}
} // namespace limb