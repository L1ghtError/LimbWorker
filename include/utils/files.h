#ifndef _FILES_H_
#define _FILES_H_

#include <vector>

#include "utils/status.h"

namespace limb {
liret loadFileContent(const char *path, std::vector<uint8_t> &buffer);
}

#endif // _FILES_H_