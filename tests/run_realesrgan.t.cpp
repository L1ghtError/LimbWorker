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

namespace {

namespace fs = std::filesystem;

constexpr auto g_inputDir = "test_input/";
constexpr auto g_outputDir = "test_output/";

size_t findProcessorByName(limb::AppBase *application, std::string_view name) {
  const size_t pc = application->processorCount();

  for (size_t i = 0; i < pc; i++) {
    if (application->processorName(i) == name) {
      return i;
    }
  }
  return -1;
}

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

inline liret saveFile(const char *path, const unsigned char *data, size_t size) {
  std::ofstream file(path, std::ios::binary);
  if (!file)
    return liret::kAborted;

  file.write(reinterpret_cast<const char *>(data), size);
  if (!file.good())
    return liret::kAborted;

  return liret::kOk;
}

} // namespace

class MockRepository : public limb::MediaRepository {
public:
  MockRepository() {}
  ~MockRepository() {}

  liret getImageById(const char *id, size_t size, unsigned char **filedata, size_t *filesize) const override {
    if (!id || !filedata || !filesize)
      return liret::kInvalidInput;

    std::string path = std::string(g_inputDir) + id;

    std::vector<uint8_t> buffer;
    liret ret = loadFile(path.c_str(), buffer);
    if (ret != liret::kOk)
      return ret;

    if (buffer.empty()) {
      *filedata = nullptr;
      *filesize = 0;
      return liret::kOk;
    }

    unsigned char *data = new unsigned char[buffer.size()];
    std::memcpy(data, buffer.data(), buffer.size());

    *filedata = data;
    *filesize = buffer.size();
    return liret::kOk;
  }

  liret updateImageById(const char *id, size_t size, unsigned char *filedata, size_t filesize) const override {
    if (!id || !filedata || filesize == 0) {
      return liret::kInvalidInput;
    }

    std::string path = std::string(g_outputDir) + id;
    return saveFile(path.c_str(), filedata, filesize);
  }
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

    // ASSERT_TRUE(fs::exists(g_inputDir)) << "test_input folder not found";

    if (!fs::exists(g_outputDir)) {
      fs::create_directories(g_outputDir);
    }

    fs::current_path(cwd.parent_path());

    const char *filepath = "input.png";

    application = std::make_unique<
        limb::App<limb::ImageService<MockRepository>, limb::ProcessorLoader, limb::CapabilitiesProvider>>(
        limb::ImageService<MockRepository>(MockRepository()), limb::ProcessorLoader({"../processors"}),
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

TEST_F(InferenceRunner, realEsrgan_verify) {
  const char pn[] = "Real-ESRGAN";
  size_t pc = findProcessorByName(application.get(), pn);
  ASSERT_NE(pc, -1) << pn << " not found!";

  auto log_delta_ms = [](float val) {
    using clock = std::chrono::steady_clock;
    static auto last = clock::now();

    auto now = clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
    last = now;

    std::fprintf(stdout, "%.2f, t = %lld ms\n", val, (long long)ms);
  };

  for (const auto &entry : fs::directory_iterator(g_inputDir)) {
    if (!entry.is_regular_file())
      continue;

    const fs::path inputPath = entry.path();
    const std::string filename = inputPath.filename().string();

    limb::ImageTask task{.modelId = uint32_t(pc), .imageId = filename};

    liret ret = application->processImage(task, [log_delta_ms](float val) { log_delta_ms(val); });
    ASSERT_EQ(ret, liret::kOk) << "Failed to process image";
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  //
  ::testing::FLAGS_gtest_catch_exceptions = false;
  return RUN_ALL_TESTS();
}