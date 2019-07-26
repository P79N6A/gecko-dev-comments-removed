









#ifndef WEBRTC_MODULES_VIDEO_CODING_SOURCE_INTERNAL_DEFINES_H_
#define WEBRTC_MODULES_VIDEO_CODING_SOURCE_INTERNAL_DEFINES_H_

#include "webrtc/typedefs.h"

namespace webrtc
{

#define MASK_32_BITS(x) (0xFFFFFFFF & (x))

inline uint32_t MaskWord64ToUWord32(int64_t w64)
{
    return static_cast<uint32_t>(MASK_32_BITS(w64));
}

#define VCM_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define VCM_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define VCM_DEFAULT_CODEC_WIDTH 352
#define VCM_DEFAULT_CODEC_HEIGHT 288
#define VCM_DEFAULT_FRAME_RATE 30
#define VCM_MIN_BITRATE 30
#define VCM_FLUSH_INDICATOR 4


#define VCM_NO_CODEC_IDX -1
#ifdef VIDEOCODEC_VP8
  #define VCM_VP8_IDX VCM_NO_CODEC_IDX + 1
#else
  #define VCM_VP8_IDX VCM_NO_CODEC_IDX
#endif
#ifdef VIDEOCODEC_I420
  #define VCM_I420_IDX VIDEOCODEC_VP8 + 1
#else
  #define VCM_I420_IDX VIDEOCODEC_VP8
#endif
#define VCM_NUM_VIDEO_CODECS_AVAILABLE VCM_I420_IDX + 1

#define VCM_NO_RECEIVER_ID 0

inline int32_t VCMId(const int32_t vcmId, const int32_t receiverId = 0)
{
    return static_cast<int32_t>((vcmId << 16) + receiverId);
}

}  

#endif 
