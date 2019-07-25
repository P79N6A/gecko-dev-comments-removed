














#ifndef LIBYUV_SOURCE_VIDEO_COMMON_H_
#define LIBYUV_SOURCE_VIDEO_COMMON_H_

#include <string>

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif







#define FOURCC(a, b, c, d) (\
    (static_cast<uint32>(a)) | (static_cast<uint32>(b) << 8) | \
    (static_cast<uint32>(c) << 16) | (static_cast<uint32>(d) << 24))




enum FourCC {
  
  FOURCC_I420 = FOURCC('I', '4', '2', '0'),
  FOURCC_I422 = FOURCC('I', '4', '2', '2'),
  FOURCC_I444 = FOURCC('I', '4', '4', '4'),
  FOURCC_I400 = FOURCC('I', '4', '0', '0'),
  FOURCC_YV12 = FOURCC('Y', 'V', '1', '2'),
  FOURCC_YV16 = FOURCC('Y', 'V', '1', '6'),
  FOURCC_YV24 = FOURCC('Y', 'V', '2', '4'),
  FOURCC_YUY2 = FOURCC('Y', 'U', 'Y', '2'),
  FOURCC_UYVY = FOURCC('U', 'Y', 'V', 'Y'),
  FOURCC_M420 = FOURCC('M', '4', '2', '0'),
  FOURCC_Q420 = FOURCC('Q', '4', '2', '0'),
  FOURCC_24BG = FOURCC('2', '4', 'B', 'G'),
  FOURCC_ABGR = FOURCC('A', 'B', 'G', 'R'),
  FOURCC_BGRA = FOURCC('B', 'G', 'R', 'A'),
  FOURCC_ARGB = FOURCC('A', 'R', 'G', 'B'),
  FOURCC_MJPG = FOURCC('M', 'J', 'P', 'G'),
  FOURCC_RAW  = FOURCC('r', 'a', 'w', ' '),
  FOURCC_NV21 = FOURCC('N', 'V', '2', '1'),
  FOURCC_NV12 = FOURCC('N', 'V', '1', '2'),
  
  
  FOURCC_RGGB = FOURCC('R', 'G', 'G', 'B'),
  FOURCC_BGGR = FOURCC('B', 'G', 'G', 'R'),
  FOURCC_GRBG = FOURCC('G', 'R', 'B', 'G'),
  FOURCC_GBRG = FOURCC('G', 'B', 'R', 'G'),

  
  
  FOURCC_IYUV = FOURCC('I', 'Y', 'U', 'V'),  
  FOURCC_YU12 = FOURCC('Y', 'U', '1', '2'),  
  FOURCC_YU16 = FOURCC('Y', 'U', '1', '6'),  
  FOURCC_YU24 = FOURCC('Y', 'U', '2', '4'),  
  FOURCC_YUYV = FOURCC('Y', 'U', 'Y', 'V'),  
  FOURCC_YUVS = FOURCC('y', 'u', 'v', 's'),  
  FOURCC_HDYC = FOURCC('H', 'D', 'Y', 'C'),  
  FOURCC_2VUY = FOURCC('2', 'v', 'u', 'y'),  
  FOURCC_JPEG = FOURCC('J', 'P', 'E', 'G'),  
  FOURCC_BA81 = FOURCC('B', 'A', '8', '1'),  
  FOURCC_RGB3 = FOURCC('R', 'G', 'B', '3'),  
  FOURCC_BGR3 = FOURCC('B', 'G', 'R', '3'),  

  
  FOURCC_ANY  = 0xFFFFFFFF,
};


uint32 CanonicalFourCC(uint32 fourcc);

#ifdef __cplusplus
}  
}  
#endif

#endif  
