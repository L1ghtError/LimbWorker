#ifndef _NCNN_SERVICE_H_
#define _NCNN_SERVICE_H_

#include <gpu.h>
#include <layer.h>
#include <net.h>

#include "utils/callbacks.h"
#include "utils/status.h"

namespace limb {
class NcnnService {
public:
  NcnnService(ncnn::Net *net, bool tta_mode = false);
  virtual ~NcnnService();

  virtual liret load() = 0;
  // TODO: Make Lambda Embty (caller passes desired logger)
  virtual liret process_matrix(
      const ncnn::Mat &inimage, ncnn::Mat &outimage,
      const ProgressCallback &&procb = [](float val) { printf("%.2f\n", val); }) const = 0;

protected:
  ncnn::Net *net;
  bool tta_mode;
};
} // namespace limb
#endif // _NCNN_SERVICE_H_