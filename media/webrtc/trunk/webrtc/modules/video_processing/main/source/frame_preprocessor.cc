









#include "webrtc/modules/video_processing/main/source/frame_preprocessor.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

VPMFramePreprocessor::VPMFramePreprocessor()
    : id_(0),
      content_metrics_(NULL),
      max_frame_rate_(0),
      resampled_frame_(),
      enable_ca_(false),
      frame_cnt_(0) {
  spatial_resampler_ = new VPMSimpleSpatialResampler();
  ca_ = new VPMContentAnalysis(true);
  vd_ = new VPMVideoDecimator();
}

VPMFramePreprocessor::~VPMFramePreprocessor() {
  Reset();
  delete spatial_resampler_;
  delete ca_;
  delete vd_;
}

int32_t VPMFramePreprocessor::ChangeUniqueId(const int32_t id) {
  id_ = id;
  return VPM_OK;
}

void  VPMFramePreprocessor::Reset() {
  ca_->Release();
  vd_->Reset();
  content_metrics_ = NULL;
  spatial_resampler_->Reset();
  enable_ca_ = false;
  frame_cnt_ = 0;
}


void  VPMFramePreprocessor::EnableTemporalDecimation(bool enable) {
  vd_->EnableTemporalDecimation(enable);
}

void VPMFramePreprocessor::EnableContentAnalysis(bool enable) {
  enable_ca_ = enable;
}

void  VPMFramePreprocessor::SetInputFrameResampleMode(
    VideoFrameResampling resampling_mode) {
  spatial_resampler_->SetInputFrameResampleMode(resampling_mode);
}

int32_t VPMFramePreprocessor::SetMaxFramerate(uint32_t max_frame_rate) {
  if (max_frame_rate == 0) return VPM_PARAMETER_ERROR;

  
  max_frame_rate_ = max_frame_rate;
  return vd_->SetMaxFramerate(max_frame_rate);
}

int32_t VPMFramePreprocessor::SetTargetResolution(
    uint32_t width, uint32_t height, uint32_t frame_rate) {
  if ( (width == 0) || (height == 0) || (frame_rate == 0)) {
    return VPM_PARAMETER_ERROR;
  }
  int32_t ret_val = 0;
  ret_val = spatial_resampler_->SetTargetFrameSize(width, height);

  if (ret_val < 0) return ret_val;

  ret_val = vd_->SetTargetframe_rate(frame_rate);
  if (ret_val < 0) return ret_val;

  return VPM_OK;
}

void VPMFramePreprocessor::UpdateIncomingframe_rate() {
  vd_->UpdateIncomingframe_rate();
}

uint32_t VPMFramePreprocessor::Decimatedframe_rate() {
  return vd_->Decimatedframe_rate();
}


uint32_t VPMFramePreprocessor::DecimatedWidth() const {
  return spatial_resampler_->TargetWidth();
}


uint32_t VPMFramePreprocessor::DecimatedHeight() const {
  return spatial_resampler_->TargetHeight();
}


int32_t VPMFramePreprocessor::PreprocessFrame(const I420VideoFrame& frame,
    I420VideoFrame** processed_frame) {
  if (frame.IsZeroSize()) {
    return VPM_PARAMETER_ERROR;
  }

  vd_->UpdateIncomingframe_rate();

  if (vd_->DropFrame()) {
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceVideo, id_,
                 "Drop frame due to frame rate");
    return 1;  
  }

  
  
  *processed_frame = NULL;
  if (spatial_resampler_->ApplyResample(frame.width(), frame.height()))  {
    int32_t ret = spatial_resampler_->ResampleFrame(frame, &resampled_frame_);
    if (ret != VPM_OK) return ret;
    *processed_frame = &resampled_frame_;
  }

  
  if (enable_ca_) {
    
    
    if (frame_cnt_ % kSkipFrameCA == 0) {
      if (*processed_frame == NULL)  {
        content_metrics_ = ca_->ComputeContentMetrics(frame);
      } else {
        content_metrics_ = ca_->ComputeContentMetrics(resampled_frame_);
      }
    }
    ++frame_cnt_;
  }
  return VPM_OK;
}

VideoContentMetrics* VPMFramePreprocessor::ContentMetrics() const {
  return content_metrics_;
}

}  
