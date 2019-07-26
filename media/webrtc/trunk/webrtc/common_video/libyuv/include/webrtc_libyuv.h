













#ifndef WEBRTC_COMMON_VIDEO_LIBYUV_INCLUDE_WEBRTC_LIBYUV_H_
#define WEBRTC_COMMON_VIDEO_LIBYUV_INCLUDE_WEBRTC_LIBYUV_H_

#include <stdio.h>

#include "common_types.h"  
#include "common_video/interface/i420_video_frame.h"
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


const double kPerfectPSNR = 48.0f;



VideoType RawVideoTypeToCommonVideoVideoType(RawVideoType type);



enum VideoRotationMode {
  kRotateNone = 0,
  kRotate90 = 90,
  kRotate180 = 180,
  kRotate270 = 270,
};






int AlignInt(int value, int alignment);








void Calc16ByteAlignedStride(int width, int* stride_y, int* stride_uv);








int CalcBufferSize(VideoType type, int width, int height);








int PrintI420VideoFrame(const I420VideoFrame& frame, FILE* file);








int ExtractBuffer(const I420VideoFrame& input_frame,
                  int size, uint8_t* buffer);













int ConvertToI420(VideoType src_video_type,
                  const uint8_t* src_frame,
                  int crop_x, int crop_y,
                  int src_width, int src_height,
                  int sample_size,
                  VideoRotationMode rotation,
                  I420VideoFrame* dst_frame);









int ConvertFromI420(const I420VideoFrame& src_frame,
                    VideoType dst_video_type, int dst_sample_size,
                    uint8_t* dst_frame);


int ConvertFromYV12(const I420VideoFrame& src_frame,
                    VideoType dst_video_type, int dst_sample_size,
                    uint8_t* dst_frame);





int ConvertRGB24ToARGB(const uint8_t* src_frame,
                       uint8_t* dst_frame,
                       int width, int height,
                       int dst_stride);
int ConvertNV12ToRGB565(const uint8_t* src_frame,
                        uint8_t* dst_frame,
                        int width, int height);









int MirrorI420LeftRight(const I420VideoFrame* src_frame,
                        I420VideoFrame* dst_frame);
int MirrorI420UpDown(const I420VideoFrame* src_frame,
                     I420VideoFrame* dst_frame);



double I420PSNR(const I420VideoFrame* ref_frame,
                const I420VideoFrame* test_frame);

double I420SSIM(const I420VideoFrame* ref_frame,
                const I420VideoFrame* test_frame);




double I420PSNR(const uint8_t* ref_frame,
                const uint8_t* test_frame,
                int width, int height);

double I420SSIM(const uint8_t* ref_frame,
                const uint8_t* test_frame,
                int width, int height);
}

#endif  
