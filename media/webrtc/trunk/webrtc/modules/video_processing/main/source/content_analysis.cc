








#include "webrtc/modules/video_processing/main/source/content_analysis.h"

#include <math.h>
#include <stdlib.h>

#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"
#include "webrtc/system_wrappers/interface/tick_util.h"

namespace webrtc {

VPMContentAnalysis::VPMContentAnalysis(bool runtime_cpu_detection)
    : orig_frame_(NULL),
      width_(0),
      height_(0),
      skip_num_(1),
      border_(8),
      motion_magnitude_(0.0f),
      spatial_pred_err_(0.0f),
      spatial_pred_err_h_(0.0f),
      spatial_pred_err_v_(0.0f),
      first_frame_(true),
      ca_Init_(false) {
  ComputeSpatialMetrics = &VPMContentAnalysis::ComputeSpatialMetrics_C;
  TemporalDiffMetric = &VPMContentAnalysis::TemporalDiffMetric_C;

  if (runtime_cpu_detection) {
#if defined(WEBRTC_ARCH_X86_FAMILY)
    if (WebRtc_GetCPUInfo(kSSE2)) {
      ComputeSpatialMetrics = &VPMContentAnalysis::ComputeSpatialMetrics_SSE2;
      TemporalDiffMetric = &VPMContentAnalysis::TemporalDiffMetric_SSE2;
    }
#endif
  }
  Release();
}

VPMContentAnalysis::~VPMContentAnalysis() {
  Release();
}


VideoContentMetrics* VPMContentAnalysis::ComputeContentMetrics(
  const I420VideoFrame& inputFrame) {
  if (inputFrame.IsZeroSize())
    return NULL;

  
  if (width_ != inputFrame.width() || height_ != inputFrame.height()) {
    if (VPM_OK != Initialize(inputFrame.width(), inputFrame.height()))
      return NULL;
  }
#if !defined(WEBRTC_GONK)
  
  if (ca_Init_) {
    
    orig_frame_ = inputFrame.buffer(kYPlane);

    
    (this->*ComputeSpatialMetrics)();

    if (first_frame_ == false) {
      ComputeMotionMetrics();
    }

    
    memcpy(prev_frame_.get(), orig_frame_, width_ * height_);

    first_frame_ =  false;
  }
#endif

  return ContentMetrics();
}

int32_t VPMContentAnalysis::Release() {
  content_metrics_.reset(NULL);
  prev_frame_.reset(NULL);

  width_ = 0;
  height_ = 0;
  first_frame_ = true;

  return VPM_OK;
}

int32_t VPMContentAnalysis::Initialize(int width, int height) {
  ca_Init_ = false;
  width_ = width;
  height_ = height;
  first_frame_ = true;

  
  
  skip_num_ = 1;

  
  if ( (height_ >=  576) && (width_ >= 704) ) {
    skip_num_ = 2;
  }
  
  if ( (height_ >=  1080) && (width_ >= 1920) ) {
    skip_num_ = 4;
  }

  content_metrics_.reset(NULL);
  prev_frame_.reset(NULL);

  
  
  if (width_ <= 32 || height_ <= 32) {
    return VPM_PARAMETER_ERROR;
  }

  content_metrics_.reset(new VideoContentMetrics());
  if (!content_metrics_) {
    return VPM_MEMORY;
  }

#if !defined(WEBRTC_GONK)
  prev_frame_.reset(new uint8_t[width_ * height_]);  
  if (!prev_frame_) {
    return VPM_MEMORY;
  }
#endif

  
  ca_Init_ = true;
  return VPM_OK;
}




int32_t VPMContentAnalysis::ComputeMotionMetrics() {
  
  
  (this->*TemporalDiffMetric)();
  return VPM_OK;
}





int32_t VPMContentAnalysis::TemporalDiffMetric_C() {
  
  int sizei = height_;
  int sizej = width_;
  uint32_t tempDiffSum = 0;
  uint32_t pixelSum = 0;
  uint64_t pixelSqSum = 0;

  uint32_t num_pixels = 0;  
  const int width_end = ((width_ - 2*border_) & -16) + border_;
  uint8_t *prev_frame = prev_frame_.get();

  for (int i = border_; i < sizei - border_; i += skip_num_) {
    for (int j = border_; j < width_end; j++) {
      num_pixels += 1;
      int ssn =  i * sizej + j;

      uint8_t currPixel  = orig_frame_[ssn];
      uint8_t prevPixel  = prev_frame[ssn];

      tempDiffSum += (uint32_t)abs((int16_t)(currPixel - prevPixel));
      pixelSum += (uint32_t) currPixel;
      pixelSqSum += (uint64_t) (currPixel * currPixel);
    }
  }

  
  motion_magnitude_ = 0.0f;

  if (tempDiffSum == 0) return VPM_OK;

  
  float const tempDiffAvg = (float)tempDiffSum / (float)(num_pixels);
  float const pixelSumAvg = (float)pixelSum / (float)(num_pixels);
  float const pixelSqSumAvg = (float)pixelSqSum / (float)(num_pixels);
  float contrast = pixelSqSumAvg - (pixelSumAvg * pixelSumAvg);

  if (contrast > 0.0) {
    contrast = sqrt(contrast);
    motion_magnitude_ = tempDiffAvg/contrast;
  }
  return VPM_OK;
}








int32_t VPMContentAnalysis::ComputeSpatialMetrics_C() {
  const int sizei = height_;
  const int sizej = width_;

  
  uint32_t pixelMSA = 0;

  uint32_t spatialErrSum = 0;
  uint32_t spatialErrVSum = 0;
  uint32_t spatialErrHSum = 0;

  
  const int width_end = ((sizej - 2*border_) & -16) + border_;

  for (int i = border_; i < sizei - border_; i += skip_num_) {
    for (int j = border_; j < width_end; j++) {
      int ssn1=  i * sizej + j;
      int ssn2 = (i + 1) * sizej + j; 
      int ssn3 = (i - 1) * sizej + j; 
      int ssn4 = i * sizej + j + 1;   
      int ssn5 = i * sizej + j - 1;   

      uint16_t refPixel1  = orig_frame_[ssn1] << 1;
      uint16_t refPixel2  = orig_frame_[ssn1] << 2;

      uint8_t bottPixel = orig_frame_[ssn2];
      uint8_t topPixel = orig_frame_[ssn3];
      uint8_t rightPixel = orig_frame_[ssn4];
      uint8_t leftPixel = orig_frame_[ssn5];

      spatialErrSum  += (uint32_t) abs((int16_t)(refPixel2
          - (uint16_t)(bottPixel + topPixel + leftPixel + rightPixel)));
      spatialErrVSum += (uint32_t) abs((int16_t)(refPixel1
          - (uint16_t)(bottPixel + topPixel)));
      spatialErrHSum += (uint32_t) abs((int16_t)(refPixel1
          - (uint16_t)(leftPixel + rightPixel)));
      pixelMSA += orig_frame_[ssn1];
    }
  }

  
  const float spatialErr = (float)(spatialErrSum >> 2);
  const float spatialErrH = (float)(spatialErrHSum >> 1);
  const float spatialErrV = (float)(spatialErrVSum >> 1);
  const float norm = (float)pixelMSA;

  
  spatial_pred_err_ = spatialErr / norm;
  
  spatial_pred_err_h_ = spatialErrH / norm;
  
  spatial_pred_err_v_ = spatialErrV / norm;
  return VPM_OK;
}

VideoContentMetrics* VPMContentAnalysis::ContentMetrics() {
  if (ca_Init_ == false) return NULL;

  if (content_metrics_) {
    content_metrics_->spatial_pred_err = spatial_pred_err_;
    content_metrics_->spatial_pred_err_h = spatial_pred_err_h_;
    content_metrics_->spatial_pred_err_v = spatial_pred_err_v_;
    
    content_metrics_->motion_magnitude = motion_magnitude_;
  }

  return content_metrics_.get();
}

}  
