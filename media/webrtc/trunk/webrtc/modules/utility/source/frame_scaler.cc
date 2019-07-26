









#include "modules/utility/source/frame_scaler.h"

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include "common_video/libyuv/include/scaler.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {

FrameScaler::FrameScaler()
    : scaler_(new Scaler()),
      scaled_frame_() {}

FrameScaler::~FrameScaler() {}

int FrameScaler::ResizeFrameIfNeeded(I420VideoFrame* video_frame,
                                     int out_width,
                                     int out_height) {
  if (video_frame->IsZeroSize()) {
    return -1;
  }

  if ((video_frame->width() != out_width) ||
      (video_frame->height() != out_height)) {
    
    scaler_->Set(video_frame->width(), video_frame->height(), out_width,
                 out_height, kI420, kI420, kScaleBox);
    int ret = scaler_->Scale(*video_frame, &scaled_frame_);
    if (ret < 0) {
      return ret;
    }

    scaled_frame_.set_render_time_ms(video_frame->render_time_ms());
    scaled_frame_.set_timestamp(video_frame->timestamp());
    video_frame->SwapFrame(&scaled_frame_);
  }
  return 0;
}

}  

#endif  
