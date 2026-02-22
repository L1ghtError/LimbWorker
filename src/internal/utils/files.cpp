#include "utils/files.h"

#include <fstream>
namespace limb {
liret loadFileContent(const char *path, std::vector<uint8_t> &buffer) {
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
} // namespace limb