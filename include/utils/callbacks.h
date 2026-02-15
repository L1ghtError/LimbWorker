#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_
#include <functional>

namespace limb {

using ProgressCallback = std::function<void(float currnetProgress)>;

inline ProgressCallback defaultProgressCallback = [](float val) {};

} // namespace limb
#endif // _CALLBACKS_H_