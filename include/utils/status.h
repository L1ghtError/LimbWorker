#ifndef _STATUS_H_
#define _STATUS_H_

static const char *ReturnStatusResolver[]{
    "Ok",             // 0
    "Unknown error",  // 1
    "Not Found",      // 2
    "Already Exists", // 3
    "Aborted",        // 4
    "Unimplemented",  // 5
    "Uninitialized",  // 6
    "Invalid input"   // 7
};

namespace limb {
enum class ReturnStatus : int {
  kOk = 0,
  kUnknown = 1,
  kNotFound = 2,
  kAlreadyExists = 3,
  kAborted = 4,
  kUnimplemented = 5,
  kUninitialized = 6,
  kInvalidInput = 7,
  kIncomplete = 8,
  kBufferTooSmall = 9,
  kOutOfMemory = 10,
};

class StatusManager {
public:
  static const char *getErrorMessage(limb::ReturnStatus retStatus) {
    return ReturnStatusResolver[static_cast<int>(retStatus)];
  };
};
} // namespace limb
using liret = limb::ReturnStatus;
using listat = limb::StatusManager;
#endif // _STATUS_H_