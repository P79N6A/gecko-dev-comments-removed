










#include "libyuv/video_common.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#define ARRAY_SIZE(x) (int)(sizeof(x) / sizeof(x[0]))

struct FourCCAliasEntry {
  uint32 alias;
  uint32 canonical;
};

static const struct FourCCAliasEntry kFourCCAliases[] = {
  {FOURCC_IYUV, FOURCC_I420},
  {FOURCC_YU16, FOURCC_I422},
  {FOURCC_YU24, FOURCC_I444},
  {FOURCC_YUYV, FOURCC_YUY2},
  {FOURCC_YUVS, FOURCC_YUY2},  
  {FOURCC_HDYC, FOURCC_UYVY},
  {FOURCC_2VUY, FOURCC_UYVY},  
  {FOURCC_JPEG, FOURCC_MJPG},  
  {FOURCC_DMB1, FOURCC_MJPG},
  {FOURCC_BA81, FOURCC_BGGR},
  {FOURCC_RGB3, FOURCC_RAW },
  {FOURCC_BGR3, FOURCC_24BG},
  {FOURCC_CM32, FOURCC_BGRA},  
  {FOURCC_CM24, FOURCC_RAW },  
  {FOURCC_L555, FOURCC_RGBO},  
  {FOURCC_L565, FOURCC_RGBP},  
  {FOURCC_5551, FOURCC_RGBO},  
};



LIBYUV_API
uint32 CanonicalFourCC(uint32 fourcc) {
  int i;
  for (i = 0; i < ARRAY_SIZE(kFourCCAliases); ++i) {
    if (kFourCCAliases[i].alias == fourcc) {
      return kFourCCAliases[i].canonical;
    }
  }
  
  return fourcc;
}

#ifdef __cplusplus
}  
}  
#endif

