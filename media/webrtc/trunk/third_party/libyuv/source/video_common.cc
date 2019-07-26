










#include "libyuv/video_common.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#define ARRAY_SIZE(x) (static_cast<int>((sizeof(x) / sizeof(x[0]))))

struct FourCCAliasEntry {
  uint32 alias;
  uint32 canonical;
};

static const FourCCAliasEntry kFourCCAliases[] = {
  {FOURCC_IYUV, FOURCC_I420},
  {FOURCC_YU12, FOURCC_I420},
  {FOURCC_YU16, FOURCC_I422},
  {FOURCC_YU24, FOURCC_I444},
  {FOURCC_YUYV, FOURCC_YUY2},
  {FOURCC_YUVS, FOURCC_YUY2},
  {FOURCC_HDYC, FOURCC_UYVY},
  {FOURCC_2VUY, FOURCC_UYVY},
  {FOURCC_BA81, FOURCC_BGGR},
  {FOURCC_JPEG, FOURCC_MJPG},  
  {FOURCC_RGB3, FOURCC_RAW},
  {FOURCC_BGR3, FOURCC_24BG},
};

LIBYUV_API
uint32 CanonicalFourCC(uint32 fourcc) {
  for (int i = 0; i < ARRAY_SIZE(kFourCCAliases); ++i) {
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

