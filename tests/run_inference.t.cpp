#include "utils/stb-wrap.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <vector>

#include "capabilities-provider.h"
#include "limb-app.h"
#include "media-repository/media-repository.hpp"
#include "processor-loader.h"
#include "utils/status.h"

size_t findProcessorByName(limb::AppBase *application, std::string_view name) {
  const size_t pc = application->processorCount();

  for (size_t i = 0; i < pc; i++) {
    if (application->processorName(i) == name) {
      return i;
    }
  }
  return -1;
}

class MockRepository : public limb::MediaRepository {
public:
  MockRepository(std::string path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
      return;

    std::size_t size = file.tellg();
    file.seekg(0);

    pixels.resize(size);
    file.read((char *)pixels.data(), size);
  }
  ~MockRepository() {}

  liret getImageById(const char *id, size_t size, unsigned char **filedata, size_t *filesize) const override {
    *filesize = pixels.size();
    if (pixels.size() <= 0) {
      return liret::kAborted;
    }

    *filedata = new unsigned char[*filesize];
    memcpy(*filedata, pixels.data(), *filesize);
    return liret::kOk;
  }

  liret updateImageById(const char *id, size_t size, unsigned char *filedata, size_t filesize) const override {
    if (!id || !filedata || filesize == 0) {
      return liret::kInvalidInput;
    }

    std::string filename(id, size);
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
      return liret::kAborted;
    }

    out.write(reinterpret_cast<const char *>(filedata), filesize);
    out.close();

    return liret::kOk;
  }

private:
  std::vector<uint8_t> pixels;
};

class InferenceRunner : public ::testing::Test {
public:
  static void SetUpTestSuite() {
    namespace fs = std::filesystem;

    fs::path cwd = fs::current_path();
    if (cwd.filename() != "tests") {
      GTEST_SKIP() << "Tests must be run from a directory that is named 'tests'. " << "Current parent is '"
                   << cwd.parent_path().filename().string() << "'";
    }

    fs::current_path(cwd.parent_path());

    const char *filepath = "input.png";

    application = std::make_unique<
        limb::App<limb::ImageService<MockRepository>, limb::ProcessorLoader, limb::CapabilitiesProvider>>(
        limb::ImageService<MockRepository>(MockRepository(filepath)), limb::ProcessorLoader({"../processors"}),
        limb::CapabilitiesProvider());

    ASSERT_EQ(application->init(), liret::kOk) << "Failed to initialize application";
  }

  static void TearDownTestSuite() {
    if (application)
      application->deinit();
  }

  static std::unique_ptr<limb::AppBase> application;
};

std::unique_ptr<limb::AppBase> InferenceRunner::application;

TEST_F(InferenceRunner, rmbg_verify) {
  const char pn[] = "RMBG-1.4";
  size_t pc = findProcessorByName(application.get(), pn);
  ASSERT_NE(pc, -1) << pn << " not found!";

  limb::ImageTask task{.imageId = "./rmbg_test_output.png"};
  task.modelId = pc;

  liret ret = application->processImage(task);
  ASSERT_EQ(ret, liret::kOk) << "Failed to process image";
}

TEST_F(InferenceRunner, realEsrgan_verify) {
  const char pn[] = "Real-ESRGAN";
  size_t pc = findProcessorByName(application.get(), pn);
  ASSERT_NE(pc, -1) << pn << " not found!";

  limb::ImageTask task{.imageId = "./realesrgan_test_output.png"};
  task.modelId = pc;

  auto log_delta_ms = [](float val) {
    using clock = std::chrono::steady_clock;
    static auto last = clock::now();

    auto now = clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
    last = now;

    std::fprintf(stdout, "%.2f, t = %lld ms\n", val, (long long)ms);
  };

  liret ret = application->processImage(task, [log_delta_ms](float val) { log_delta_ms(val); });
  ASSERT_EQ(ret, liret::kOk) << "Failed to process image";
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  //
  ::testing::FLAGS_gtest_catch_exceptions = false;
  return RUN_ALL_TESTS();
}