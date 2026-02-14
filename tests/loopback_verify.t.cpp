#include "utils/stb-wrap.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <limits>
#include <memory>
#include <string_view>
#include <vector>

#include "capabilities-provider.h"
#include "limb-app.h"
#include "media-repository/media-repository.hpp"
#include "processor-loader.h"
#include "utils/status.h"

// PNG 20x20 where 10 top pixels is gray and bottom 10 is black
const unsigned char imageData[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x14, 0x08, 0x06, 0x00, 0x00, 0x00, 0x8D, 0x89, 0x1D, 0x0D, 0x00, 0x00, 0x00, 0x01, 0x73,
    0x52, 0x47, 0x42, 0x00, 0xAE, 0xCE, 0x1C, 0xE9, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xB1,
    0x8F, 0x0B, 0xFC, 0x61, 0x05, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0xC3, 0x00, 0x00,
    0x0E, 0xC3, 0x01, 0xC7, 0x6F, 0xA8, 0x64, 0x00, 0x00, 0x00, 0x3A, 0x49, 0x44, 0x41, 0x54, 0x38, 0x4F, 0xED, 0xCC,
    0xA1, 0x11, 0x00, 0x30, 0x0C, 0xC3, 0x40, 0x6F, 0xEE, 0xD1, 0xDB, 0x2B, 0xAD, 0xA0, 0x02, 0x03, 0xDE, 0x40, 0xC0,
    0x69, 0x7B, 0x26, 0xE5, 0x0F, 0xD6, 0x1E, 0x7A, 0x7B, 0xE8, 0x25, 0xC9, 0x9B, 0x49, 0x08, 0x16, 0x82, 0x85, 0x60,
    0x21, 0x58, 0x08, 0x16, 0x82, 0x85, 0x60, 0x21, 0x58, 0x08, 0xCA, 0x05, 0x44, 0xC8, 0xB8, 0x37, 0xCD, 0x15, 0xD3,
    0xD6, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

std::vector<unsigned char> dbImageData;

size_t findProcessorByName(limb::AppBase *application, std::string_view name) {
  const size_t pc = application->processorCount();

  for (size_t i = 0; i < pc; i++) {
    if (application->processorName(i) == name) {
      return i;
    }
  }
  return -1;
}

bool isImageDataIdentical() {
  const int differenceThreshold = 5;

  auto stbiDeleter = [](uint8_t *ptr) { stbi_image_free(ptr); };

  int oW, oH, oC;
  std::unique_ptr<uint8_t[], decltype(stbiDeleter)> original(
      stbi_load_from_memory(imageData, sizeof(imageData), &oW, &oH, &oC, 0), stbiDeleter);

  int pW, pH, pC;
  std::unique_ptr<uint8_t[], decltype(stbiDeleter)> processed(
      stbi_load_from_memory(dbImageData.data(), dbImageData.size(), &pW, &pH, &pC, 0), stbiDeleter);

  if (!original || !processed) {
    return false;
  }

  if (oW != pW || oH != pH) {
    return false;
  }

  for (size_t i = 0; i < oW * oH; i++) {
    // Real-world codecs can slightly distort image data, this allows for small differences
    if (std::abs(original[i] - processed[i]) > differenceThreshold) {
      return false;
    }
  }

  return true;
}

class MockRepository : public limb::MediaRepository {
public:
  ~MockRepository() {}

  liret getImageById(const char *id, size_t size, unsigned char **filedata, size_t *filesize) const override {
    *filesize = sizeof(imageData);
    *filedata = new unsigned char[*filesize];
    memcpy(*filedata, imageData, *filesize);
    return liret::kOk;
  }

  liret updateImageById(const char *id, size_t size, unsigned char *filedata, size_t filesize) const override {
    dbImageData.clear();
    if (dbImageData.capacity() < filesize) {
      dbImageData.reserve(filesize);
    }
    for (size_t i = 0; i < filesize; i++) {
      dbImageData.emplace_back(filedata[i]);
    }
    return liret::kOk;
  }
};

class LoopbackVerify : public ::testing::Test {
  void SetUp() override {
    namespace fs = std::filesystem;

    fs::path cwd = fs::current_path();
    if (cwd.filename() != "tests") {
      GTEST_SKIP() << "Tests must be run from a directory that is named 'tests'. " << "Current parent is '"
                   << cwd.parent_path().filename().string() << "'";
    }

    application = std::make_unique<
        limb::App<limb::ImageService<MockRepository>, limb::ProcessorLoader, limb::CapabilitiesProvider>>(
        limb::ImageService<MockRepository>(MockRepository()), limb::ProcessorLoader({"../processors"}),
        limb::CapabilitiesProvider());

    application->init();
  }

  void TearDown() override { application->deinit(); }

protected:
  std::unique_ptr<limb::AppBase> application;
};

TEST_F(LoopbackVerify, loopback_verify) {
  const char pn[] = "Loopback-Processor";
  size_t pc = findProcessorByName(application.get(), pn);
  ASSERT_NE(pc, -1) << pn << " not found!";

  limb::ImageTask pbody;
  pbody.modelId = pc;
  liret ret = application->processImage(pbody);
  ASSERT_EQ(ret, liret::kOk) << "Failed to process image";

  bool isDataIdentical = isImageDataIdentical();
  ASSERT_TRUE(isDataIdentical) << "Loopback processor provided wrong image data";
}

TEST_F(LoopbackVerify, invalid_processor_index) {
  const size_t count = application->processorCount();
  // Increase if necessary ;)
  ASSERT_LT(count, 2000) << "count: " << count << " is too large!";

  limb::ImageTask pbody;

  pbody.modelId = count;
  liret ret = application->processImage(pbody);
  ASSERT_NE(ret, liret::kOk) << " application processed with invalid index!";

  pbody.modelId = std::numeric_limits<uint32_t>::max();
  ret = application->processImage(pbody);
  ASSERT_NE(ret, liret::kOk) << " application processed with invalid index!";
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}