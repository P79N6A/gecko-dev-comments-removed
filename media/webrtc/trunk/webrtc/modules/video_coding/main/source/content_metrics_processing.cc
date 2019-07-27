









#include "webrtc/modules/video_coding/main/source/content_metrics_processing.h"

#include <math.h>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"

namespace webrtc {




VCMContentMetricsProcessing::VCMContentMetricsProcessing()
    : recursive_avg_factor_(1 / 150.0f),  
      frame_cnt_uniform_avg_(0),
      avg_motion_level_(0.0f),
      avg_spatial_level_(0.0f) {
  recursive_avg_ = new VideoContentMetrics();
  uniform_avg_ = new VideoContentMetrics();
}

VCMContentMetricsProcessing::~VCMContentMetricsProcessing() {
  delete recursive_avg_;
  delete uniform_avg_;
}

int VCMContentMetricsProcessing::Reset() {
  recursive_avg_->Reset();
  uniform_avg_->Reset();
  frame_cnt_uniform_avg_ = 0;
  avg_motion_level_  = 0.0f;
  avg_spatial_level_ = 0.0f;
  return VCM_OK;
}

void VCMContentMetricsProcessing::UpdateFrameRate(float frameRate) {
  
  recursive_avg_factor_ = static_cast<float> (1000.0f) /
      static_cast<float>(frameRate *  kQmMinIntervalMs);
}

VideoContentMetrics* VCMContentMetricsProcessing::LongTermAvgData() {
  return recursive_avg_;
}

VideoContentMetrics* VCMContentMetricsProcessing::ShortTermAvgData() {
  if (frame_cnt_uniform_avg_ == 0) {
    return NULL;
  }
  
  uniform_avg_->motion_magnitude = avg_motion_level_ /
      static_cast<float>(frame_cnt_uniform_avg_);
  uniform_avg_->spatial_pred_err = avg_spatial_level_ /
      static_cast<float>(frame_cnt_uniform_avg_);
  return uniform_avg_;
}

void VCMContentMetricsProcessing::ResetShortTermAvgData() {
  
  avg_motion_level_ = 0.0f;
  avg_spatial_level_ = 0.0f;
  frame_cnt_uniform_avg_ = 0;
}

int VCMContentMetricsProcessing::UpdateContentData(
    const VideoContentMetrics *contentMetrics) {
  if (contentMetrics == NULL) {
    return VCM_OK;
  }
  return ProcessContent(contentMetrics);
}

int VCMContentMetricsProcessing::ProcessContent(
    const VideoContentMetrics *contentMetrics) {
  
  
  UpdateRecursiveAvg(contentMetrics);
  
  
  UpdateUniformAvg(contentMetrics);
  return VCM_OK;
}

void VCMContentMetricsProcessing::UpdateUniformAvg(
    const VideoContentMetrics *contentMetrics) {
  
  frame_cnt_uniform_avg_ += 1;
  
  avg_motion_level_ += contentMetrics->motion_magnitude;
  avg_spatial_level_ +=  contentMetrics->spatial_pred_err;
  return;
}

void VCMContentMetricsProcessing::UpdateRecursiveAvg(
    const VideoContentMetrics *contentMetrics) {

  
  recursive_avg_->spatial_pred_err = (1 - recursive_avg_factor_) *
      recursive_avg_->spatial_pred_err +
      recursive_avg_factor_ * contentMetrics->spatial_pred_err;

  recursive_avg_->spatial_pred_err_h = (1 - recursive_avg_factor_) *
      recursive_avg_->spatial_pred_err_h +
      recursive_avg_factor_ * contentMetrics->spatial_pred_err_h;

  recursive_avg_->spatial_pred_err_v = (1 - recursive_avg_factor_) *
      recursive_avg_->spatial_pred_err_v +
      recursive_avg_factor_ * contentMetrics->spatial_pred_err_v;

  
  recursive_avg_->motion_magnitude = (1 - recursive_avg_factor_) *
      recursive_avg_->motion_magnitude +
      recursive_avg_factor_ * contentMetrics->motion_magnitude;
}
}  
