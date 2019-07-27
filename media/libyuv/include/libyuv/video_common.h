











#ifndef INCLUDE_LIBYUV_VIDEO_COMMON_H_  
#define INCLUDE_LIBYUV_VIDEO_COMMON_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif








#ifdef __cplusplus
#define FOURCC(a, b, c, d) ( \
    (static_cast<uint32>(a)) | (static_cast<uint32>(b) << 8) | \
    (static_cast<uint32>(c) << 16) | (static_cast<uint32>(d) << 24))
#else
#define FOURCC(a, b, c, d) ( \
    ((uint32)(a)) | ((uint32)(b) << 8) | /* NOLINT */ \
    ((uint32)(c) << 16) | ((uint32)(d) << 24))  /* NOLINT */
#endif












enum FourCC {
  
  FOURCC_I420 = FOURCC('I', '4', '2', '0'),
  FOURCC_I422 = FOURCC('I', '4', '2', '2'),
  FOURCC_I444 = FOURCC('I', '4', '4', '4'),
  FOURCC_I411 = FOURCC('I', '4', '1', '1'),
  FOURCC_I400 = FOURCC('I', '4', '0', '0'),
  FOURCC_NV21 = FOURCC('N', 'V', '2', '1'),
  FOURCC_NV12 = FOURCC('N', 'V', '1', '2'),
  FOURCC_YUY2 = FOURCC('Y', 'U', 'Y', '2'),
  FOURCC_UYVY = FOURCC('U', 'Y', 'V', 'Y'),

  
  FOURCC_M420 = FOURCC('M', '4', '2', '0'),
  FOURCC_Q420 = FOURCC('Q', '4', '2', '0'),

  
  FOURCC_ARGB = FOURCC('A', 'R', 'G', 'B'),
  FOURCC_BGRA = FOURCC('B', 'G', 'R', 'A'),
  FOURCC_ABGR = FOURCC('A', 'B', 'G', 'R'),
  FOURCC_24BG = FOURCC('2', '4', 'B', 'G'),
  FOURCC_RAW  = FOURCC('r', 'a', 'w', ' '),
  FOURCC_RGBA = FOURCC('R', 'G', 'B', 'A'),
  FOURCC_RGBP = FOURCC('R', 'G', 'B', 'P'),  
  FOURCC_RGBO = FOURCC('R', 'G', 'B', 'O'),  
  FOURCC_R444 = FOURCC('R', '4', '4', '4'),  

  
  FOURCC_RGGB = FOURCC('R', 'G', 'G', 'B'),
  FOURCC_BGGR = FOURCC('B', 'G', 'G', 'R'),
  FOURCC_GRBG = FOURCC('G', 'R', 'B', 'G'),
  FOURCC_GBRG = FOURCC('G', 'B', 'R', 'G'),

  
  FOURCC_MJPG = FOURCC('M', 'J', 'P', 'G'),

  
  FOURCC_YV12 = FOURCC('Y', 'V', '1', '2'),
  FOURCC_YV16 = FOURCC('Y', 'V', '1', '6'),
  FOURCC_YV24 = FOURCC('Y', 'V', '2', '4'),
  FOURCC_YU12 = FOURCC('Y', 'U', '1', '2'),  
  FOURCC_J420 = FOURCC('J', '4', '2', '0'),
  FOURCC_J400 = FOURCC('J', '4', '0', '0'),

  
  FOURCC_IYUV = FOURCC('I', 'Y', 'U', 'V'),  
  FOURCC_YU16 = FOURCC('Y', 'U', '1', '6'),  
  FOURCC_YU24 = FOURCC('Y', 'U', '2', '4'),  
  FOURCC_YUYV = FOURCC('Y', 'U', 'Y', 'V'),  
  FOURCC_YUVS = FOURCC('y', 'u', 'v', 's'),  
  FOURCC_HDYC = FOURCC('H', 'D', 'Y', 'C'),  
  FOURCC_2VUY = FOURCC('2', 'v', 'u', 'y'),  
  FOURCC_JPEG = FOURCC('J', 'P', 'E', 'G'),  
  FOURCC_DMB1 = FOURCC('d', 'm', 'b', '1'),  
  FOURCC_BA81 = FOURCC('B', 'A', '8', '1'),  
  FOURCC_RGB3 = FOURCC('R', 'G', 'B', '3'),  
  FOURCC_BGR3 = FOURCC('B', 'G', 'R', '3'),  
  FOURCC_CM32 = FOURCC(0, 0, 0, 32),  
  FOURCC_CM24 = FOURCC(0, 0, 0, 24),  
  FOURCC_L555 = FOURCC('L', '5', '5', '5'),  
  FOURCC_L565 = FOURCC('L', '5', '6', '5'),  
  FOURCC_5551 = FOURCC('5', '5', '5', '1'),  

  
  FOURCC_H264 = FOURCC('H', '2', '6', '4'),

  
  FOURCC_ANY = -1,
};

enum FourCCBpp {
  
  FOURCC_BPP_I420 = 12,
  FOURCC_BPP_I422 = 16,
  FOURCC_BPP_I444 = 24,
  FOURCC_BPP_I411 = 12,
  FOURCC_BPP_I400 = 8,
  FOURCC_BPP_NV21 = 12,
  FOURCC_BPP_NV12 = 12,
  FOURCC_BPP_YUY2 = 16,
  FOURCC_BPP_UYVY = 16,
  FOURCC_BPP_M420 = 12,
  FOURCC_BPP_Q420 = 12,
  FOURCC_BPP_ARGB = 32,
  FOURCC_BPP_BGRA = 32,
  FOURCC_BPP_ABGR = 32,
  FOURCC_BPP_RGBA = 32,
  FOURCC_BPP_24BG = 24,
  FOURCC_BPP_RAW  = 24,
  FOURCC_BPP_RGBP = 16,
  FOURCC_BPP_RGBO = 16,
  FOURCC_BPP_R444 = 16,
  FOURCC_BPP_RGGB = 8,
  FOURCC_BPP_BGGR = 8,
  FOURCC_BPP_GRBG = 8,
  FOURCC_BPP_GBRG = 8,
  FOURCC_BPP_YV12 = 12,
  FOURCC_BPP_YV16 = 16,
  FOURCC_BPP_YV24 = 24,
  FOURCC_BPP_YU12 = 12,
  FOURCC_BPP_J420 = 12,
  FOURCC_BPP_J400 = 8,
  FOURCC_BPP_MJPG = 0,  
  FOURCC_BPP_H264 = 0,
  FOURCC_BPP_IYUV = 12,
  FOURCC_BPP_YU16 = 16,
  FOURCC_BPP_YU24 = 24,
  FOURCC_BPP_YUYV = 16,
  FOURCC_BPP_YUVS = 16,
  FOURCC_BPP_HDYC = 16,
  FOURCC_BPP_2VUY = 16,
  FOURCC_BPP_JPEG = 1,
  FOURCC_BPP_DMB1 = 1,
  FOURCC_BPP_BA81 = 8,
  FOURCC_BPP_RGB3 = 24,
  FOURCC_BPP_BGR3 = 24,
  FOURCC_BPP_CM32 = 32,
  FOURCC_BPP_CM24 = 24,

  
  FOURCC_BPP_ANY  = 0,  
};


LIBYUV_API uint32 CanonicalFourCC(uint32 fourcc);

#ifdef __cplusplus
}  
}  
#endif

#endif  
