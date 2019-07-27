









#include "webrtc/modules/video_coding/main/source/qm_select.h"

#include <math.h>
#ifdef ANDROID
#include <android/log.h>
#endif

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/qm_select_data.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {



VCMQmMethod::VCMQmMethod()
    : content_metrics_(NULL),
      width_(0),
      height_(0),
      user_frame_rate_(0.0f),
      native_width_(0),
      native_height_(0),
      native_frame_rate_(0.0f),
      image_type_(kVGA),
      framerate_level_(kFrameRateHigh),
      init_(false) {
  ResetQM();
}

VCMQmMethod::~VCMQmMethod() {
}

void VCMQmMethod::ResetQM() {
  aspect_ratio_ = 1.0f;
  motion_.Reset();
  spatial_.Reset();
  content_class_ = 0;
}

uint8_t VCMQmMethod::ComputeContentClass() {
  ComputeMotionNFD();
  ComputeSpatial();
  return content_class_ = 3 * motion_.level + spatial_.level;
}

void VCMQmMethod::UpdateContent(const VideoContentMetrics*  contentMetrics) {
  content_metrics_ = contentMetrics;
}

void VCMQmMethod::ComputeMotionNFD() {
#if defined(WEBRTC_GONK)
  motion_.value = (kHighMotionNfd + kLowMotionNfd)/2;
  motion_.level = kDefault;
#else
  if (content_metrics_) {
    motion_.value = content_metrics_->motion_magnitude;
  }
  
  if (motion_.value < kLowMotionNfd) {
    motion_.level = kLow;
  } else if (motion_.value > kHighMotionNfd) {
    motion_.level  = kHigh;
  } else {
    motion_.level = kDefault;
  }
#endif
}

void VCMQmMethod::ComputeSpatial() {
#if defined(WEBRTC_GONK)
  float scale2 = image_type_ > kVGA ? kScaleTexture : 1.0;
  spatial_.value = (kHighTexture + kLowTexture)*scale2/2;
  spatial_.level = kDefault;
#else
  float spatial_err = 0.0;
  float spatial_err_h = 0.0;
  float spatial_err_v = 0.0;
  if (content_metrics_) {
    spatial_err =  content_metrics_->spatial_pred_err;
    spatial_err_h = content_metrics_->spatial_pred_err_h;
    spatial_err_v = content_metrics_->spatial_pred_err_v;
  }
  
  spatial_.value = (spatial_err + spatial_err_h + spatial_err_v) / 3.0f;

  
  float scale2 = image_type_ > kVGA ? kScaleTexture : 1.0;

  if (spatial_.value > scale2 * kHighTexture) {
    spatial_.level = kHigh;
  } else if (spatial_.value < scale2 * kLowTexture) {
    spatial_.level = kLow;
  } else {
    spatial_.level = kDefault;
  }
#endif
}

ImageType VCMQmMethod::GetImageType(uint16_t width,
                                    uint16_t height) {
  
  uint32_t image_size = width * height;
  if (image_size == kSizeOfImageType[kQCIF]) {
    return kQCIF;
  } else if (image_size == kSizeOfImageType[kHCIF]) {
    return kHCIF;
  } else if (image_size == kSizeOfImageType[kQVGA]) {
    return kQVGA;
  } else if (image_size == kSizeOfImageType[kCIF]) {
    return kCIF;
  } else if (image_size == kSizeOfImageType[kHVGA]) {
    return kHVGA;
  } else if (image_size == kSizeOfImageType[kVGA]) {
    return kVGA;
  } else if (image_size == kSizeOfImageType[kQFULLHD]) {
    return kQFULLHD;
  } else if (image_size == kSizeOfImageType[kWHD]) {
    return kWHD;
  } else if (image_size == kSizeOfImageType[kFULLHD]) {
    return kFULLHD;
  } else {
    
    return FindClosestImageType(width, height);
  }
}

ImageType VCMQmMethod::FindClosestImageType(uint16_t width, uint16_t height) {
  float size = static_cast<float>(width * height);
  float min = size;
  int isel = 0;
  for (int i = 0; i < kNumImageTypes; ++i) {
    float dist = fabs(size - kSizeOfImageType[i]);
    if (dist < min) {
      min = dist;
      isel = i;
    }
  }
  return static_cast<ImageType>(isel);
}

FrameRateLevelClass VCMQmMethod::FrameRateLevel(float avg_framerate) {
  if (avg_framerate <= kLowFrameRate) {
    return kFrameRateLow;
  } else if (avg_framerate <= kMiddleFrameRate) {
    return kFrameRateMiddle1;
  } else if (avg_framerate <= kHighFrameRate) {
     return kFrameRateMiddle2;
  } else {
    return kFrameRateHigh;
  }
}



VCMQmResolution::VCMQmResolution()
    :  qm_(new VCMResolutionScale()) {
  Reset();
}

VCMQmResolution::~VCMQmResolution() {
  delete qm_;
}

void VCMQmResolution::ResetRates() {
  sum_target_rate_ = 0.0f;
  sum_incoming_framerate_ = 0.0f;
  sum_rate_MM_ = 0.0f;
  sum_rate_MM_sgn_ = 0.0f;
  sum_packet_loss_ = 0.0f;
  buffer_level_ = kInitBufferLevel * target_bitrate_;
  frame_cnt_ = 0;
  frame_cnt_delta_ = 0;
  low_buffer_cnt_ = 0;
  update_rate_cnt_ = 0;
}

void VCMQmResolution::ResetDownSamplingState() {
  state_dec_factor_spatial_ = 1.0;
  state_dec_factor_temporal_  = 1.0;
  for (int i = 0; i < kDownActionHistorySize; i++) {
    down_action_history_[i].spatial = kNoChangeSpatial;
    down_action_history_[i].temporal = kNoChangeTemporal;
  }
}

void VCMQmResolution::Reset() {
  target_bitrate_ = 0.0f;
  incoming_framerate_ = 0.0f;
  buffer_level_ = 0.0f;
  per_frame_bandwidth_ = 0.0f;
  avg_target_rate_ = 0.0f;
  avg_incoming_framerate_ = 0.0f;
  avg_ratio_buffer_low_ = 0.0f;
  avg_rate_mismatch_ = 0.0f;
  avg_rate_mismatch_sgn_ = 0.0f;
  avg_packet_loss_ = 0.0f;
  encoder_state_ = kStableEncoding;
  num_layers_ = 1;
  ResetRates();
  ResetDownSamplingState();
  ResetQM();
}

EncoderState VCMQmResolution::GetEncoderState() {
  return encoder_state_;
}



int VCMQmResolution::Initialize(float bitrate,
                                float user_framerate,
                                uint16_t width,
                                uint16_t height,
                                int num_layers) {
  WEBRTC_TRACE(webrtc::kTraceDebug,
               webrtc::kTraceVideoCoding,
               -1,
               "qm_select.cc:initialize: %f %f %u %u",
               bitrate, user_framerate, width, height);

  if (user_framerate == 0.0f || width == 0 || height == 0) {
    return VCM_PARAMETER_ERROR;
  }
  Reset();
  target_bitrate_ = bitrate;
  incoming_framerate_ = user_framerate;
  UpdateCodecParameters(user_framerate, width, height);
  native_width_ = width;
  native_height_ = height;
  native_frame_rate_ = user_framerate;
  num_layers_ = num_layers;
  
  buffer_level_ = kInitBufferLevel * target_bitrate_;
  
  per_frame_bandwidth_ = target_bitrate_ / user_framerate;
  init_  = true;
  return VCM_OK;
}

void VCMQmResolution::UpdateCodecParameters(float frame_rate, uint16_t width,
                                            uint16_t height) {
  width_ = width;
  height_ = height;
  
  user_frame_rate_ = frame_rate;
  image_type_ = GetImageType(width, height);
}


void VCMQmResolution::UpdateEncodedSize(int encoded_size,
                                        FrameType encoded_frame_type) {
  frame_cnt_++;
  
  float encoded_size_kbits = static_cast<float>((encoded_size * 8.0) / 1000.0);

  
  
  
  
  buffer_level_ += per_frame_bandwidth_ - encoded_size_kbits;

  
  
  if (buffer_level_ <= kPercBufferThr * kInitBufferLevel * target_bitrate_) {
    low_buffer_cnt_++;
  }
}


void VCMQmResolution::UpdateRates(float target_bitrate,
                                  float encoder_sent_rate,
                                  float incoming_framerate,
                                  uint8_t packet_loss) {
  
  
  sum_target_rate_ += target_bitrate_;
  update_rate_cnt_++;

  
  sum_packet_loss_ += static_cast<float>(packet_loss / 255.0);

  
  
  
  
  float diff = target_bitrate_ - encoder_sent_rate;
  if (target_bitrate_ > 0.0)
    sum_rate_MM_ += fabs(diff) / target_bitrate_;
  int sgnDiff = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
  
  sum_rate_MM_sgn_ += sgnDiff;

  
  
  target_bitrate_ =  target_bitrate;
  incoming_framerate_ = incoming_framerate;
  sum_incoming_framerate_ += incoming_framerate_;
  
  
  per_frame_bandwidth_  = 0.0f;
  if (incoming_framerate_ > 0.0f) {
    per_frame_bandwidth_ = target_bitrate_ / incoming_framerate_;
  }
}














int VCMQmResolution::SelectResolution(VCMResolutionScale** qm) {
  if (!init_) {
    return VCM_UNINITIALIZED;
  }
  if (content_metrics_ == NULL) {
    Reset();
    *qm =  qm_;
    return VCM_OK;
  }

  
  assert(state_dec_factor_spatial_ >= 1.0f);
  assert(state_dec_factor_temporal_ >= 1.0f);
  assert(state_dec_factor_spatial_ <= kMaxSpatialDown);
  assert(state_dec_factor_temporal_ <= kMaxTempDown);
  assert(state_dec_factor_temporal_ * state_dec_factor_spatial_ <=
         kMaxTotalDown);

  
  content_class_ = ComputeContentClass();
  
  ComputeRatesForSelection();

  
  ComputeEncoderState();

  
  SetDefaultAction();
  *qm = qm_;

  
  
  if (down_action_history_[0].spatial != kNoChangeSpatial ||
      down_action_history_[0].temporal != kNoChangeTemporal) {
    if (GoingUpResolution()) {
      *qm = qm_;
      return VCM_OK;
    }
  }

  
  if (GoingDownResolution()) {
    *qm = qm_;
    return VCM_OK;
  }
  return VCM_OK;
}

void VCMQmResolution::SetDefaultAction() {
  qm_->codec_width = width_;
  qm_->codec_height = height_;
  qm_->frame_rate = user_frame_rate_;
  qm_->change_resolution_spatial = false;
  qm_->change_resolution_temporal = false;
  qm_->spatial_width_fact = 1.0f;
  qm_->spatial_height_fact = 1.0f;
  qm_->temporal_fact = 1.0f;
  action_.spatial = kNoChangeSpatial;
  action_.temporal = kNoChangeTemporal;
}

void VCMQmResolution::ComputeRatesForSelection() {
  avg_target_rate_ = 0.0f;
  avg_incoming_framerate_ = 0.0f;
  avg_ratio_buffer_low_ = 0.0f;
  avg_rate_mismatch_ = 0.0f;
  avg_rate_mismatch_sgn_ = 0.0f;
  avg_packet_loss_ = 0.0f;
  if (frame_cnt_ > 0) {
    avg_ratio_buffer_low_ = static_cast<float>(low_buffer_cnt_) /
        static_cast<float>(frame_cnt_);
  }
  if (update_rate_cnt_ > 0) {
    avg_rate_mismatch_ = static_cast<float>(sum_rate_MM_) /
        static_cast<float>(update_rate_cnt_);
    avg_rate_mismatch_sgn_ = static_cast<float>(sum_rate_MM_sgn_) /
        static_cast<float>(update_rate_cnt_);
    avg_target_rate_ = static_cast<float>(sum_target_rate_) /
        static_cast<float>(update_rate_cnt_);
    avg_incoming_framerate_ = static_cast<float>(sum_incoming_framerate_) /
        static_cast<float>(update_rate_cnt_);
    avg_packet_loss_ =  static_cast<float>(sum_packet_loss_) /
        static_cast<float>(update_rate_cnt_);
  }
  
  
  avg_target_rate_ = kWeightRate * avg_target_rate_ +
      (1.0 - kWeightRate) * target_bitrate_;
  avg_incoming_framerate_ = kWeightRate * avg_incoming_framerate_ +
      (1.0 - kWeightRate) * incoming_framerate_;
  
  assert(num_layers_ > 0);
  framerate_level_ = FrameRateLevel(
      avg_incoming_framerate_ / static_cast<float>(1 << (num_layers_ - 1)));
}

void VCMQmResolution::ComputeEncoderState() {
  
  encoder_state_ = kStableEncoding;

  
  
  
  if ((avg_ratio_buffer_low_ > kMaxBufferLow) ||
      ((avg_rate_mismatch_ > kMaxRateMisMatch) &&
          (avg_rate_mismatch_sgn_ < -kRateOverShoot))) {
    encoder_state_ = kStressedEncoding;
    WEBRTC_TRACE(webrtc::kTraceDebug,
                 webrtc::kTraceVideoCoding,
                 -1,
                 "ComputeEncoderState==Stressed");
    return;
  }
  
  
  
  if ((avg_rate_mismatch_ > kMaxRateMisMatch) &&
      (avg_rate_mismatch_sgn_ > kRateUnderShoot)) {
    encoder_state_ = kEasyEncoding;
    WEBRTC_TRACE(webrtc::kTraceDebug,
                 webrtc::kTraceVideoCoding,
                 -1,
                 "ComputeEncoderState==Easy");
    return;
  }

  WEBRTC_TRACE(webrtc::kTraceDebug,
               webrtc::kTraceVideoCoding,
               -1,
               "ComputeEncoderState==Stable");
}

bool VCMQmResolution::GoingUpResolution() {
  
  if (loadstate_ == kLoadStressed) {
    return false;
  }

  

  float fac_width = kFactorWidthSpatial[down_action_history_[0].spatial];
  float fac_height = kFactorHeightSpatial[down_action_history_[0].spatial];
  float fac_temp = kFactorTemporal[down_action_history_[0].temporal];
  
  
  
  if (down_action_history_[0].spatial == kOneQuarterSpatialUniform) {
    fac_width = kFactorWidthSpatial[kOneQuarterSpatialUniform] /
        kFactorWidthSpatial[kOneHalfSpatialUniform];
    fac_height = kFactorHeightSpatial[kOneQuarterSpatialUniform] /
        kFactorHeightSpatial[kOneHalfSpatialUniform];
  }

  
  if (down_action_history_[0].spatial != kNoChangeSpatial &&
      down_action_history_[0].temporal != kNoChangeTemporal) {
    if (ConditionForGoingUp(fac_width, fac_height, fac_temp,
                            kTransRateScaleUpSpatialTemp)) {
      action_.spatial = down_action_history_[0].spatial;
      action_.temporal = down_action_history_[0].temporal;
      UpdateDownsamplingState(kUpResolution);
      return true;
    }
  }
  
  bool selected_up_spatial = false;
  bool selected_up_temporal = false;
  if (down_action_history_[0].spatial != kNoChangeSpatial) {
    selected_up_spatial = ConditionForGoingUp(fac_width, fac_height, 1.0f,
                                              kTransRateScaleUpSpatial);
  }
  if (down_action_history_[0].temporal != kNoChangeTemporal) {
    selected_up_temporal = ConditionForGoingUp(1.0f, 1.0f, fac_temp,
                                               kTransRateScaleUpTemp);
  }
  if (selected_up_spatial && !selected_up_temporal) {
    action_.spatial = down_action_history_[0].spatial;
    action_.temporal = kNoChangeTemporal;
    UpdateDownsamplingState(kUpResolution);
    return true;
  } else if (!selected_up_spatial && selected_up_temporal) {
    action_.spatial = kNoChangeSpatial;
    action_.temporal = down_action_history_[0].temporal;
    UpdateDownsamplingState(kUpResolution);
    return true;
  } else if (selected_up_spatial && selected_up_temporal) {
    PickSpatialOrTemporal();
    UpdateDownsamplingState(kUpResolution);
    return true;
  }
  return false;
}

bool VCMQmResolution::ConditionForGoingUp(float fac_width,
                                          float fac_height,
                                          float fac_temp,
                                          float scale_fac) {
  float estimated_transition_rate_up = GetTransitionRate(fac_width, fac_height,
                                                         fac_temp, scale_fac);
  
  
  
  if (((avg_target_rate_ > estimated_transition_rate_up) &&
      (encoder_state_ == kStableEncoding)) ||
      (encoder_state_ == kEasyEncoding)) {
    return true;
  } else {
    return false;
  }
}

bool VCMQmResolution::GoingDownResolution() {
  float estimated_transition_rate_down =
      GetTransitionRate(1.0f, 1.0f, 1.0f, 1.0f);
  float max_rate = kFrameRateFac[framerate_level_] * kMaxRateQm[image_type_];

  WEBRTC_TRACE(webrtc::kTraceDebug,
               webrtc::kTraceVideoCoding,
               -1,
               "state %d avg_target_rate %f estimated_trans_rate_down %f max %f",
               loadstate_, avg_target_rate_, estimated_transition_rate_down, max_rate
               );

  
  
  
  if (loadstate_ == kLoadStressed
      || (avg_target_rate_ < estimated_transition_rate_down)
      || (encoder_state_ == kStressedEncoding && avg_target_rate_ < max_rate)) {
    
    
    uint8_t spatial_fact =
        kSpatialAction[content_class_ +
                       9 * RateClass(estimated_transition_rate_down)];
    uint8_t temp_fact =
        kTemporalAction[content_class_ +
                        9 * RateClass(estimated_transition_rate_down)];

    switch (spatial_fact) {
      case 4: {
        action_.spatial = kOneQuarterSpatialUniform;
        break;
      }
      case 2: {
        action_.spatial = kOneHalfSpatialUniform;
        break;
      }
      case 1: {
        action_.spatial = kNoChangeSpatial;
        break;
      }
      default: {
        assert(false);
      }
    }
    switch (temp_fact) {
      case 3: {
        action_.temporal = kTwoThirdsTemporal;
        break;
      }
      case 2: {
        action_.temporal = kOneHalfTemporal;
        break;
      }
      case 1: {
        action_.temporal = kNoChangeTemporal;
        break;
      }
      default: {
        assert(false);
      }
    }
    
    assert(action_.temporal == kNoChangeTemporal ||
           action_.spatial == kNoChangeSpatial);

    
    
    if (loadstate_ == kLoadStressed
        && action_.temporal == kNoChangeTemporal
        && action_.spatial == kNoChangeSpatial) {
      
      if (avg_incoming_framerate_ >= 40) {
        action_.temporal = kOneHalfTemporal;
      } else if (avg_incoming_framerate_ >= 24) {
        action_.temporal = kTwoThirdsTemporal;
      } else {
        
        action_.spatial = kOneHalfSpatialUniform;
      }
    }

    
    
    AdjustAction();

    
    if (action_.spatial != kNoChangeSpatial ||
        action_.temporal != kNoChangeTemporal) {
      UpdateDownsamplingState(kDownResolution);
      return true;
    }
  }
  return false;
}

float VCMQmResolution::GetTransitionRate(float fac_width,
                                         float fac_height,
                                         float fac_temp,
                                         float scale_fac) {
  ImageType image_type = GetImageType(
      static_cast<uint16_t>(fac_width * width_),
      static_cast<uint16_t>(fac_height * height_));

  FrameRateLevelClass framerate_level =
      FrameRateLevel(fac_temp * avg_incoming_framerate_);
  
  
  if (down_action_history_[1].temporal == kNoChangeTemporal &&
      fac_temp > 1.0f) {
    framerate_level = FrameRateLevel(native_frame_rate_);
  }

  
  
  float max_rate = kFrameRateFac[framerate_level] * kMaxRateQm[image_type];

  uint8_t image_class = image_type > kVGA ? 1: 0;
  uint8_t table_index = image_class * 9 + content_class_;
  
  
  float scaleTransRate = kScaleTransRateQm[table_index];
  
  return static_cast<float> (scale_fac * scaleTransRate * max_rate);
}

void VCMQmResolution::UpdateDownsamplingState(UpDownAction up_down) {
  if (up_down == kUpResolution) {
    qm_->spatial_width_fact = 1.0f / kFactorWidthSpatial[action_.spatial];
    qm_->spatial_height_fact = 1.0f / kFactorHeightSpatial[action_.spatial];
    
    
    if (action_.spatial == kOneQuarterSpatialUniform) {
      qm_->spatial_width_fact =
          1.0f * kFactorWidthSpatial[kOneHalfSpatialUniform] /
          kFactorWidthSpatial[kOneQuarterSpatialUniform];
      qm_->spatial_height_fact =
          1.0f * kFactorHeightSpatial[kOneHalfSpatialUniform] /
          kFactorHeightSpatial[kOneQuarterSpatialUniform];
    }
    qm_->temporal_fact = 1.0f / kFactorTemporal[action_.temporal];
    RemoveLastDownAction();
  } else if (up_down == kDownResolution) {
    ConstrainAmountOfDownSampling();
    ConvertSpatialFractionalToWhole();
    qm_->spatial_width_fact = kFactorWidthSpatial[action_.spatial];
    qm_->spatial_height_fact = kFactorHeightSpatial[action_.spatial];
    qm_->temporal_fact = kFactorTemporal[action_.temporal];
    InsertLatestDownAction();
  } else {
    
    
    assert(false);
  }
  UpdateCodecResolution();
  state_dec_factor_spatial_ = state_dec_factor_spatial_ *
      qm_->spatial_width_fact * qm_->spatial_height_fact;
  state_dec_factor_temporal_ = state_dec_factor_temporal_ * qm_->temporal_fact;
}

void  VCMQmResolution::UpdateCodecResolution() {
  if (action_.spatial != kNoChangeSpatial) {
    qm_->change_resolution_spatial = true;
    int old_width = qm_->codec_width;
    int old_height = qm_->codec_height;
    qm_->codec_width = static_cast<uint16_t>(width_ /
                                             qm_->spatial_width_fact + 0.5f);
    qm_->codec_height = static_cast<uint16_t>(height_ /
                                              qm_->spatial_height_fact + 0.5f);
    
    assert(qm_->codec_width <= native_width_);
    assert(qm_->codec_height <= native_height_);
    
    
    assert(qm_->codec_width % 2 == 0);
    assert(qm_->codec_height % 2 == 0);
    WEBRTC_TRACE(webrtc::kTraceDebug,
                 webrtc::kTraceVideoCoding,
                 -1,
                 "UpdateCodecResolution: [%d %d] %d %d => %d %d",
                 native_width_, native_height_,
                 old_width, old_height,
                 qm_->codec_width, qm_->codec_height
                 );
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "WebRTC",
                        "UpdateCodecResolution: [%d %d] %d %d => %d %d",
                        native_width_, native_height_,
                        old_width, old_height,
                        qm_->codec_width, qm_->codec_height);
#endif
  }
  if (action_.temporal != kNoChangeTemporal) {
    qm_->change_resolution_temporal = true;
    float old_rate = qm_->frame_rate;
    
    qm_->frame_rate = avg_incoming_framerate_ / qm_->temporal_fact + 0.5f;
    if (down_action_history_[0].temporal == 0) {
      
      
      
      
      qm_->frame_rate = native_frame_rate_;
    }
    WEBRTC_TRACE(webrtc::kTraceDebug,
                 webrtc::kTraceVideoCoding,
                 -1,
                 "UpdateCodecResolution: [%f] %f fps => %f fps",
                 native_frame_rate_,
                 old_rate,
                 qm_->frame_rate
                 );
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "WebRTC",
                        "UpdateCodecResolution: [%f] %f fps => %f fps",
                        native_frame_rate_,
                        old_rate,
                        qm_->frame_rate);
#endif

  }
}

uint8_t VCMQmResolution::RateClass(float transition_rate) {
  return avg_target_rate_ < (kFacLowRate * transition_rate) ? 0:
  (avg_target_rate_ >= transition_rate ? 2 : 1);
}



void VCMQmResolution::AdjustAction() {
  
  
  
  if (spatial_.level == kDefault && motion_.level != kHigh &&
      action_.spatial != kNoChangeSpatial &&
      framerate_level_ == kFrameRateHigh) {
    action_.spatial = kNoChangeSpatial;
    action_.temporal = kTwoThirdsTemporal;
  }
  
  
  
  if (motion_.level == kLow && spatial_.level == kLow &&
      framerate_level_ <= kFrameRateMiddle1 &&
      action_.temporal != kNoChangeTemporal) {
    action_.spatial = kOneHalfSpatialUniform;
    action_.temporal = kNoChangeTemporal;
  }
  
  
  
  if (action_.spatial != kNoChangeSpatial &&
      down_action_history_[0].spatial == kOneQuarterSpatialUniform &&
      framerate_level_ != kFrameRateLow) {
    action_.spatial = kNoChangeSpatial;
    action_.temporal = kTwoThirdsTemporal;
  }
  
  if (num_layers_ > 2) {
    if (action_.temporal !=  kNoChangeTemporal) {
      action_.spatial = kOneHalfSpatialUniform;
    }
    action_.temporal = kNoChangeTemporal;
  }
  
  
  if (action_.spatial != kNoChangeSpatial &&
      !EvenFrameSize()) {
    action_.spatial = kNoChangeSpatial;
    
    
    action_.temporal = kTwoThirdsTemporal;
  }
}

void VCMQmResolution::ConvertSpatialFractionalToWhole() {
  
  
  
  if (action_.spatial == kOneHalfSpatialUniform) {
    bool found = false;
    int isel = kDownActionHistorySize;
    for (int i = 0; i < kDownActionHistorySize; ++i) {
      if (down_action_history_[i].spatial ==  kOneHalfSpatialUniform) {
        isel = i;
        found = true;
        break;
      }
    }
    if (found) {
       action_.spatial = kOneQuarterSpatialUniform;
       state_dec_factor_spatial_ = state_dec_factor_spatial_ /
           (kFactorWidthSpatial[kOneHalfSpatialUniform] *
            kFactorHeightSpatial[kOneHalfSpatialUniform]);
       
       ConstrainAmountOfDownSampling();
       if (action_.spatial == kNoChangeSpatial) {
         
         action_.spatial = kOneHalfSpatialUniform;
         state_dec_factor_spatial_ = state_dec_factor_spatial_ *
             kFactorWidthSpatial[kOneHalfSpatialUniform] *
             kFactorHeightSpatial[kOneHalfSpatialUniform];
       } else {
         
         
         for (int i = isel; i < kDownActionHistorySize - 1; ++i) {
           down_action_history_[i].spatial =
               down_action_history_[i + 1].spatial;
         }
         width_ = width_ * kFactorWidthSpatial[kOneHalfSpatialUniform];
         height_ = height_ * kFactorHeightSpatial[kOneHalfSpatialUniform];
       }
    }
  }
}



bool VCMQmResolution::EvenFrameSize() {
  if (action_.spatial == kOneHalfSpatialUniform) {
    if ((width_ * 3 / 4) % 2 != 0 || (height_ * 3 / 4) % 2 != 0) {
      return false;
    }
  } else if (action_.spatial == kOneQuarterSpatialUniform) {
    if ((width_ * 1 / 2) % 2 != 0 || (height_ * 1 / 2) % 2 != 0) {
      return false;
    }
  }
  return true;
}

void VCMQmResolution::InsertLatestDownAction() {
  if (action_.spatial != kNoChangeSpatial) {
    for (int i = kDownActionHistorySize - 1; i > 0; --i) {
      down_action_history_[i].spatial = down_action_history_[i - 1].spatial;
    }
    down_action_history_[0].spatial = action_.spatial;
  }
  if (action_.temporal != kNoChangeTemporal) {
    for (int i = kDownActionHistorySize - 1; i > 0; --i) {
      down_action_history_[i].temporal = down_action_history_[i - 1].temporal;
    }
    down_action_history_[0].temporal = action_.temporal;
  }
}

void VCMQmResolution::RemoveLastDownAction() {
  if (action_.spatial != kNoChangeSpatial) {
    
    if (action_.spatial == kOneQuarterSpatialUniform) {
      down_action_history_[0].spatial = kOneHalfSpatialUniform;
    } else {
      for (int i = 0; i < kDownActionHistorySize - 1; ++i) {
        down_action_history_[i].spatial = down_action_history_[i + 1].spatial;
      }
      down_action_history_[kDownActionHistorySize - 1].spatial =
          kNoChangeSpatial;
    }
  }
  if (action_.temporal != kNoChangeTemporal) {
    for (int i = 0; i < kDownActionHistorySize - 1; ++i) {
      down_action_history_[i].temporal = down_action_history_[i + 1].temporal;
    }
    down_action_history_[kDownActionHistorySize - 1].temporal =
        kNoChangeTemporal;
  }
}

void VCMQmResolution::ConstrainAmountOfDownSampling() {
  
  
  

  float spatial_width_fact = kFactorWidthSpatial[action_.spatial];
  float spatial_height_fact = kFactorHeightSpatial[action_.spatial];
  float temporal_fact = kFactorTemporal[action_.temporal];
  float new_dec_factor_spatial = state_dec_factor_spatial_ *
      spatial_width_fact * spatial_height_fact;
  float new_dec_factor_temp = state_dec_factor_temporal_ * temporal_fact;

  
  
  if ((width_ * height_) <= kMinImageSize ||
      new_dec_factor_spatial > kMaxSpatialDown) {
    action_.spatial = kNoChangeSpatial;
    new_dec_factor_spatial = state_dec_factor_spatial_;
  }
  
  
  if (avg_incoming_framerate_ <= kMinFrameRate ||
      new_dec_factor_temp > kMaxTempDown) {
    action_.temporal = kNoChangeTemporal;
    new_dec_factor_temp = state_dec_factor_temporal_;
  }
  
  
  if (new_dec_factor_spatial * new_dec_factor_temp > kMaxTotalDown) {
    if (action_.spatial != kNoChangeSpatial) {
      action_.spatial = kNoChangeSpatial;
    } else if (action_.temporal != kNoChangeTemporal) {
      action_.temporal = kNoChangeTemporal;
    } else {
      
      
      
      
      
      assert(false);
    }
  }
}

void VCMQmResolution::PickSpatialOrTemporal() {
  
  if (state_dec_factor_spatial_ > state_dec_factor_temporal_) {
    action_.spatial = down_action_history_[0].spatial;
    action_.temporal = kNoChangeTemporal;
  } else {
    action_.spatial = kNoChangeSpatial;
    action_.temporal = down_action_history_[0].temporal;
  }
}


void VCMQmResolution::SelectSpatialDirectionMode(float transition_rate) {
  
  
  if (avg_target_rate_ < transition_rate * kRateRedSpatial2X2) {
    qm_->spatial_width_fact = 2.0f;
    qm_->spatial_height_fact = 2.0f;
  }
  
  float spatial_err = 0.0f;
  float spatial_err_h = 0.0f;
  float spatial_err_v = 0.0f;
  if (content_metrics_) {
    spatial_err = content_metrics_->spatial_pred_err;
    spatial_err_h = content_metrics_->spatial_pred_err_h;
    spatial_err_v = content_metrics_->spatial_pred_err_v;
  }

  
  if (aspect_ratio_ >= 16.0f / 9.0f) {
    
    if (spatial_err_h < spatial_err && spatial_err_h < spatial_err_v) {
      qm_->spatial_width_fact = 2.0f;
      qm_->spatial_height_fact = 1.0f;
    }
  }
  
  if (spatial_err < spatial_err_h * (1.0f + kSpatialErr2x2VsHoriz) &&
      spatial_err < spatial_err_v * (1.0f + kSpatialErr2X2VsVert)) {
    qm_->spatial_width_fact = 4.0f / 3.0f;
    qm_->spatial_height_fact = 4.0f / 3.0f;
  }
  
  if (spatial_err_v < spatial_err_h * (1.0f - kSpatialErrVertVsHoriz) &&
      spatial_err_v < spatial_err * (1.0f - kSpatialErr2X2VsVert)) {
    qm_->spatial_width_fact = 1.0f;
    qm_->spatial_height_fact = 2.0f;
  }
}

void VCMQmResolution::SetCPULoadState(CPULoadState state) {
  loadstate_ = state;
}



VCMQmRobustness::VCMQmRobustness() {
  Reset();
}

VCMQmRobustness::~VCMQmRobustness() {
}

void VCMQmRobustness::Reset() {
  prev_total_rate_ = 0.0f;
  prev_rtt_time_ = 0;
  prev_packet_loss_ = 0;
  prev_code_rate_delta_ = 0;
  ResetQM();
}




float VCMQmRobustness::AdjustFecFactor(uint8_t code_rate_delta,
                                       float total_rate,
                                       float framerate,
                                       uint32_t rtt_time,
                                       uint8_t packet_loss) {
  
  float adjust_fec =  1.0f;
  if (content_metrics_ == NULL) {
    return adjust_fec;
  }
  
  ComputeMotionNFD();
  ComputeSpatial();

  

  
  
  prev_total_rate_ = total_rate;
  prev_rtt_time_ = rtt_time;
  prev_packet_loss_ = packet_loss;
  prev_code_rate_delta_ = code_rate_delta;
  return adjust_fec;
}


bool VCMQmRobustness::SetUepProtection(uint8_t code_rate_delta,
                                       float total_rate,
                                       uint8_t packet_loss,
                                       bool frame_type) {
  
  return false;
}

}  
