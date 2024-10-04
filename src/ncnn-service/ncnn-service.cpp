#include "ncnn-service/ncnn-service.h"
namespace limb {
NcnnService::NcnnService(ncnn::Net *_net, bool _tta_mode) {
  net = _net;
  tta_mode = _tta_mode;
}
NcnnService::~NcnnService() {}

} // namespace limb