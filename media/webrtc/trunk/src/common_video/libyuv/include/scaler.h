













#ifndef WEBRTC_COMMON_VIDEO_LIBYUV_INCLUDE_SCALER_H_
#define WEBRTC_COMMON_VIDEO_LIBYUV_INCLUDE_SCALER_H_

#include "common_video/libyuv/include/libyuv.h"
#include "typedefs.h"

namespace webrtc {


enum ScaleMethod {
  kScalePoint,  
  kScaleBilinear,
  kScaleBox
};



class Scaler {
 public:
  Scaler();
  ~Scaler();

  
  
  
  
  int Set(int src_width, int src_height,
          int dst_width, int dst_height,
          VideoType src_video_type, VideoType dst_video_type,
          ScaleMethod method);

  
  
  
  
  
  
  int Scale(const uint8_t* src_frame,
            uint8_t*& dst_frame,
            int& dst_size);

 private:
  
  bool SupportedVideoType(VideoType src_video_type,
                          VideoType dst_video_type);

  ScaleMethod   method_;
  int           src_width_;
  int           src_height_;
  int           dst_width_;
  int           dst_height_;
  bool          set_;
};

}  

#endif  
