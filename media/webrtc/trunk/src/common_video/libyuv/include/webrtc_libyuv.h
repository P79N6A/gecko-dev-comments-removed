













#ifndef WEBRTC_COMMON_VIDEO_LIBYUV_INCLUDE_WEBRTC_LIBYUV_H_
#define WEBRTC_COMMON_VIDEO_LIBYUV_INCLUDE_WEBRTC_LIBYUV_H_

#include "common_types.h"  
#include "typedefs.h"

namespace webrtc {





enum VideoType {
  kUnknown,
  kI420,
  kIYUV,
  kRGB24,
  kABGR,
  kARGB,
  kARGB4444,
  kRGB565,
  kARGB1555,
  kYUY2,
  kYV12,
  kUYVY,
  kMJPG,
  kNV21,
  kNV12,
  kBGRA,
};



VideoType RawVideoTypeToCommonVideoVideoType(RawVideoType type);



enum VideoRotationMode {
  kRotateNone = 0,
  kRotate90 = 90,
  kRotate180 = 180,
  kRotate270 = 270,
};








int CalcBufferSize(VideoType type, int width, int height);















int ConvertToI420(VideoType src_video_type,
                  const uint8_t* src_frame,
                  int crop_x, int crop_y,
                  int src_width, int src_height,
                  int sample_size,
                  int dst_width, int dst_height, int dst_stride,
                  VideoRotationMode rotation,
                  uint8_t* dst_frame);











int ConvertFromI420(const uint8_t* src_frame, int src_stride,
                    VideoType dst_video_type, int dst_sample_size,
                    int width, int height,
                    uint8_t* dst_frame);


int ConvertFromYV12(const uint8_t* src_frame, int src_stride,
                    VideoType dst_video_type, int dst_sample_size,
                    int width, int height,
                    uint8_t* dst_frame);





int ConvertRGB24ToARGB(const uint8_t* src_frame,
                       uint8_t* dst_frame,
                       int width, int height,
                       int dst_stride);
int ConvertNV12ToRGB565(const uint8_t* src_frame,
                        uint8_t* dst_frame,
                        int width, int height);










int MirrorI420LeftRight(const uint8_t* src_frame,
                        uint8_t* dst_frame,
                        int width, int height);
int MirrorI420UpDown(const uint8_t* src_frame,
                     uint8_t* dst_frame,
                     int width, int height);


double I420PSNR(const uint8_t* ref_frame,
                const uint8_t* test_frame,
                int width, int height);

double I420SSIM(const uint8_t* ref_frame,
                const uint8_t* test_frame,
                int width, int height);
}

#endif  
