









#include "webrtc/modules/video_processing/main/source/spatial_resampler.h"


namespace webrtc {

VPMSimpleSpatialResampler::VPMSimpleSpatialResampler()
    : resampling_mode_(kFastRescaling),
      target_width_(0),
      target_height_(0),
      scaler_() {}

VPMSimpleSpatialResampler::~VPMSimpleSpatialResampler() {}


int32_t VPMSimpleSpatialResampler::SetTargetFrameSize(int32_t width,
                                                      int32_t height) {
  if (resampling_mode_ == kNoRescaling) return VPM_OK;

  if (width < 1 || height < 1) return VPM_PARAMETER_ERROR;

  target_width_ = width;
  target_height_ = height;

  return VPM_OK;
}

void VPMSimpleSpatialResampler::SetInputFrameResampleMode(
    VideoFrameResampling resampling_mode) {
  resampling_mode_ = resampling_mode;
}

void VPMSimpleSpatialResampler::Reset() {
  resampling_mode_ = kFastRescaling;
  target_width_ = 0;
  target_height_ = 0;
}

int32_t VPMSimpleSpatialResampler::ResampleFrame(const I420VideoFrame& inFrame,
                                                 I420VideoFrame* outFrame) {
  
  if (resampling_mode_ == kNoRescaling)
     return VPM_OK;
  
  else if ((inFrame.width() == target_width_) &&
    (inFrame.height() == target_height_))  {
    return VPM_OK;
  }

  
  
  
  int ret_val = 0;
  ret_val = scaler_.Set(inFrame.width(), inFrame.height(),
                       target_width_, target_height_, kI420, kI420, kScaleBox);
  if (ret_val < 0)
    return ret_val;

  ret_val = scaler_.Scale(inFrame, outFrame);

  
  
  outFrame->set_timestamp(inFrame.timestamp());
  outFrame->set_render_time_ms(inFrame.render_time_ms());

  if (ret_val == 0)
    return VPM_OK;
  else
    return VPM_SCALE_ERROR;
}

int32_t VPMSimpleSpatialResampler::TargetHeight() {
  return target_height_;
}

int32_t VPMSimpleSpatialResampler::TargetWidth() {
  return target_width_;
}

bool VPMSimpleSpatialResampler::ApplyResample(int32_t width,
                                              int32_t height) {
  if ((width == target_width_ && height == target_height_) ||
       resampling_mode_ == kNoRescaling)
    return false;
  else
    return true;
}

}  
