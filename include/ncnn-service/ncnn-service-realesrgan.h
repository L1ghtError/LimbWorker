#ifndef _REALESRGAN_SERVICE_H_
#define _REALESRGAN_SERVICE_H_
#include "ncnn-service/ncnn-service.h"

namespace limb {
class RealesrganService : public NcnnService {
public:
  RealesrganService(ncnn::Net *net, bool tta_mode = false);
  ~RealesrganService();
  liret load() override;
  liret process_matrix(const ncnn::Mat &inimage, ncnn::Mat &outimage, const ProgressCallback &&procb) const override;

public:
  int scale;
  int tilesize;
  int prepadding;

private:
  ncnn::Pipeline *realesrgan_preproc;
  ncnn::Pipeline *realesrgan_postproc;
  ncnn::Layer *bicubic_2x;
  ncnn::Layer *bicubic_3x;
  ncnn::Layer *bicubic_4x;
};
} // namespace limb
#endif _REALESRGAN_SERVICE_H_