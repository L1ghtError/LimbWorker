#include "utils/files.h"

#include <fstream>
namespace limb {
liret loadFileContent(const char *path, std::vector<uint8_t> &buffer) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    return liret::kNotFound;

  std::size_t size = file.tellg();
  if (size < 0)
    return liret::kAborted;

  file.seekg(0);

  buffer.resize(size);
  file.read((char *)buffer.data(), size);
  return liret::kOk;
}
} // namespace limb