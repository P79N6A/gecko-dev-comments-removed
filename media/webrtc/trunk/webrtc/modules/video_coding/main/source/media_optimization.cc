









#include "webrtc/modules/video_coding/main/source/media_optimization.h"

#include "webrtc/modules/video_coding/main/source/content_metrics_processing.h"
#include "webrtc/modules/video_coding/main/source/qm_select.h"
#include "webrtc/modules/video_coding/utility/include/frame_dropper.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {
namespace media_optimization {

MediaOptimization::MediaOptimization(int32_t id, Clock* clock)
    : id_(id),
      clock_(clock),
      max_bit_rate_(0),
      send_codec_type_(kVideoCodecUnknown),
      codec_width_(0),
      codec_height_(0),
      user_frame_rate_(0),
      frame_dropper_(new FrameDropper),
      loss_prot_logic_(
          new VCMLossProtectionLogic(clock_->TimeInMilliseconds())),
      fraction_lost_(0),
      send_statistics_zero_encode_(0),
      max_payload_size_(1460),
      target_bit_rate_(0),
      incoming_frame_rate_(0),
      enable_qm_(false),
      video_protection_callback_(NULL),
      video_qmsettings_callback_(NULL),
      encoded_frame_samples_(),
      avg_sent_bit_rate_bps_(0),
      avg_sent_framerate_(0),
      key_frame_cnt_(0),
      delta_frame_cnt_(0),
      content_(new VCMContentMetricsProcessing()),
      qm_resolution_(new VCMQmResolution()),
      last_qm_update_time_(0),
      last_change_time_(0),
      num_layers_(0) {
  memset(send_statistics_, 0, sizeof(send_statistics_));
  memset(incoming_frame_times_, -1, sizeof(incoming_frame_times_));
}

MediaOptimization::~MediaOptimization(void) {
  loss_prot_logic_->Release();
}

int32_t MediaOptimization::Reset() {
  memset(incoming_frame_times_, -1, sizeof(incoming_frame_times_));
  incoming_frame_rate_ = 0.0;
  frame_dropper_->Reset();
  loss_prot_logic_->Reset(clock_->TimeInMilliseconds());
  frame_dropper_->SetRates(0, 0);
  content_->Reset();
  qm_resolution_->Reset();
  loss_prot_logic_->UpdateFrameRate(incoming_frame_rate_);
  loss_prot_logic_->Reset(clock_->TimeInMilliseconds());
  send_statistics_zero_encode_ = 0;
  target_bit_rate_ = 0;
  codec_width_ = 0;
  codec_height_ = 0;
  user_frame_rate_ = 0;
  key_frame_cnt_ = 0;
  delta_frame_cnt_ = 0;
  last_qm_update_time_ = 0;
  last_change_time_ = 0;
  encoded_frame_samples_.clear();
  avg_sent_bit_rate_bps_ = 0;
  num_layers_ = 1;
  return VCM_OK;
}

uint32_t MediaOptimization::SetTargetRates(uint32_t target_bitrate,
                                           uint8_t fraction_lost,
                                           uint32_t round_trip_time_ms) {
  
  
  if (max_bit_rate_ > 0 &&
      target_bitrate > static_cast<uint32_t>(max_bit_rate_)) {
    target_bitrate = max_bit_rate_;
  }
  VCMProtectionMethod* selected_method = loss_prot_logic_->SelectedMethod();
  float target_bitrate_kbps = static_cast<float>(target_bitrate) / 1000.0f;
  loss_prot_logic_->UpdateBitRate(target_bitrate_kbps);
  loss_prot_logic_->UpdateRtt(round_trip_time_ms);
  loss_prot_logic_->UpdateResidualPacketLoss(static_cast<float>(fraction_lost));

  
  float actual_frame_rate = SentFrameRate();

  
  if (actual_frame_rate < 1.0) {
    actual_frame_rate = 1.0;
  }

  
  
  loss_prot_logic_->UpdateFrameRate(actual_frame_rate);

  fraction_lost_ = fraction_lost;

  
  
  
  
  FilterPacketLossMode filter_mode = kMaxFilter;
  uint8_t packet_loss_enc = loss_prot_logic_->FilteredLoss(
      clock_->TimeInMilliseconds(), filter_mode, fraction_lost);

  
  loss_prot_logic_->UpdateFilteredLossPr(packet_loss_enc);

  
  uint32_t protection_overhead_bps = 0;

  
  float sent_video_rate_kbps = 0.0f;
  if (selected_method) {
    
    selected_method->UpdateContentMetrics(content_->ShortTermAvgData());

    
    
    
    loss_prot_logic_->UpdateMethod();

    
    uint32_t sent_video_rate_bps = 0;
    uint32_t sent_nack_rate_bps = 0;
    uint32_t sent_fec_rate_bps = 0;
    
    
    
    UpdateProtectionCallback(selected_method,
                             &sent_video_rate_bps,
                             &sent_nack_rate_bps,
                             &sent_fec_rate_bps);
    uint32_t sent_total_rate_bps =
        sent_video_rate_bps + sent_nack_rate_bps + sent_fec_rate_bps;
    
    
    if (sent_total_rate_bps > 0) {
      protection_overhead_bps = static_cast<uint32_t>(
          target_bitrate *
              static_cast<double>(sent_nack_rate_bps + sent_fec_rate_bps) /
              sent_total_rate_bps +
          0.5);
    }
    
    if (protection_overhead_bps > target_bitrate / 2)
      protection_overhead_bps = target_bitrate / 2;

    
    
    packet_loss_enc = selected_method->RequiredPacketLossER();
    sent_video_rate_kbps = static_cast<float>(sent_video_rate_bps) / 1000.0f;
  }

  
  target_bit_rate_ = target_bitrate - protection_overhead_bps;

  
  float target_video_bitrate_kbps =
      static_cast<float>(target_bit_rate_) / 1000.0f;
  frame_dropper_->SetRates(target_video_bitrate_kbps, incoming_frame_rate_);

  if (enable_qm_) {
    
    qm_resolution_->UpdateRates(target_video_bitrate_kbps,
                                sent_video_rate_kbps,
                                incoming_frame_rate_,
                                fraction_lost_);
    
    bool select_qm = CheckStatusForQMchange();
    if (select_qm) {
      SelectQuality();
    }
    
    content_->ResetShortTermAvgData();
  }

  return target_bit_rate_;
}

int32_t MediaOptimization::SetEncodingData(VideoCodecType send_codec_type,
                                           int32_t max_bit_rate,
                                           uint32_t frame_rate,
                                           uint32_t target_bitrate,
                                           uint16_t width,
                                           uint16_t height,
                                           int num_layers) {
  
  
  
  
  last_change_time_ = clock_->TimeInMilliseconds();
  content_->Reset();
  content_->UpdateFrameRate(frame_rate);

  max_bit_rate_ = max_bit_rate;
  send_codec_type_ = send_codec_type;
  target_bit_rate_ = target_bitrate;
  float target_bitrate_kbps = static_cast<float>(target_bitrate) / 1000.0f;
  loss_prot_logic_->UpdateBitRate(target_bitrate_kbps);
  loss_prot_logic_->UpdateFrameRate(static_cast<float>(frame_rate));
  loss_prot_logic_->UpdateFrameSize(width, height);
  loss_prot_logic_->UpdateNumLayers(num_layers);
  frame_dropper_->Reset();
  frame_dropper_->SetRates(target_bitrate_kbps, static_cast<float>(frame_rate));
  user_frame_rate_ = static_cast<float>(frame_rate);
  codec_width_ = width;
  codec_height_ = height;
  num_layers_ = (num_layers <= 1) ? 1 : num_layers;  
  int32_t ret = VCM_OK;
  ret = qm_resolution_->Initialize(target_bitrate_kbps,
                                   user_frame_rate_,
                                   codec_width_,
                                   codec_height_,
                                   num_layers_);
  return ret;
}

void MediaOptimization::EnableProtectionMethod(bool enable,
                                               VCMProtectionMethodEnum method) {
  bool updated = false;
  if (enable) {
    updated = loss_prot_logic_->SetMethod(method);
  } else {
    loss_prot_logic_->RemoveMethod(method);
  }
  if (updated) {
    loss_prot_logic_->UpdateMethod();
  }
}

bool MediaOptimization::IsProtectionMethodEnabled(
    VCMProtectionMethodEnum method) {
  return (loss_prot_logic_->SelectedType() == method);
}

uint32_t MediaOptimization::InputFrameRate() {
  ProcessIncomingFrameRate(clock_->TimeInMilliseconds());
  return uint32_t(incoming_frame_rate_ + 0.5f);
}

uint32_t MediaOptimization::SentFrameRate() {
  PurgeOldFrameSamples(clock_->TimeInMilliseconds());
  UpdateSentFramerate();
  return avg_sent_framerate_;
}

uint32_t MediaOptimization::SentBitRate() {
  const int64_t now_ms = clock_->TimeInMilliseconds();
  PurgeOldFrameSamples(now_ms);
  UpdateSentBitrate(now_ms);
  return avg_sent_bit_rate_bps_;
}

int32_t MediaOptimization::UpdateWithEncodedData(int encoded_length,
                                                 uint32_t timestamp,
                                                 FrameType encoded_frame_type) {
  const int64_t now_ms = clock_->TimeInMilliseconds();
  PurgeOldFrameSamples(now_ms);
  if (encoded_frame_samples_.size() > 0 &&
      encoded_frame_samples_.back().timestamp == timestamp) {
    
    
    
    encoded_frame_samples_.back().size_bytes += encoded_length;
    encoded_frame_samples_.back().time_complete_ms = now_ms;
  } else {
    encoded_frame_samples_.push_back(
        EncodedFrameSample(encoded_length, timestamp, now_ms));
  }
  UpdateSentBitrate(now_ms);
  UpdateSentFramerate();
  if (encoded_length > 0) {
    const bool delta_frame = (encoded_frame_type != kVideoFrameKey &&
                              encoded_frame_type != kVideoFrameGolden);

    frame_dropper_->Fill(encoded_length, delta_frame);
    if (max_payload_size_ > 0 && encoded_length > 0) {
      const float min_packets_per_frame =
          encoded_length / static_cast<float>(max_payload_size_);
      if (delta_frame) {
        loss_prot_logic_->UpdatePacketsPerFrame(min_packets_per_frame,
                                                clock_->TimeInMilliseconds());
      } else {
        loss_prot_logic_->UpdatePacketsPerFrameKey(
            min_packets_per_frame, clock_->TimeInMilliseconds());
      }

      if (enable_qm_) {
        
        qm_resolution_->UpdateEncodedSize(encoded_length, encoded_frame_type);
      }
    }
    if (!delta_frame && encoded_length > 0) {
      loss_prot_logic_->UpdateKeyFrameSize(static_cast<float>(encoded_length));
    }

    
    if (delta_frame) {
      delta_frame_cnt_++;
    } else {
      key_frame_cnt_++;
    }
  }

  return VCM_OK;
}

int32_t MediaOptimization::RegisterProtectionCallback(
    VCMProtectionCallback* protection_callback) {
  video_protection_callback_ = protection_callback;
  return VCM_OK;
}

int32_t MediaOptimization::RegisterVideoQMCallback(
    VCMQMSettingsCallback* video_qmsettings) {
  video_qmsettings_callback_ = video_qmsettings;
  
  if (video_qmsettings_callback_ != NULL) {
    enable_qm_ = true;
  } else {
    enable_qm_ = false;
  }
  return VCM_OK;
}

void MediaOptimization::EnableFrameDropper(bool enable) {
  frame_dropper_->Enable(enable);
}

bool MediaOptimization::DropFrame() {
  
  frame_dropper_->Leak((uint32_t)(InputFrameRate() + 0.5f));

  return frame_dropper_->DropFrame();
}

int32_t MediaOptimization::SentFrameCount(VCMFrameCount* frame_count) const {
  frame_count->numDeltaFrames = delta_frame_cnt_;
  frame_count->numKeyFrames = key_frame_cnt_;
  return VCM_OK;
}

void MediaOptimization::UpdateIncomingFrameRate() {
  int64_t now = clock_->TimeInMilliseconds();
  if (incoming_frame_times_[0] == 0) {
    
  } else {
    
    for (int32_t i = (kFrameCountHistorySize - 2); i >= 0; i--) {
      incoming_frame_times_[i + 1] = incoming_frame_times_[i];
    }
  }
  incoming_frame_times_[0] = now;
  ProcessIncomingFrameRate(now);
}

void MediaOptimization::UpdateContentData(
    const VideoContentMetrics* content_metrics) {
  
  if (content_metrics == NULL) {
    
    enable_qm_ = false;
    qm_resolution_->Reset();
  } else {
    content_->UpdateContentData(content_metrics);
  }
}

int32_t MediaOptimization::SelectQuality() {
  
  qm_resolution_->ResetQM();

  
  qm_resolution_->UpdateContent(content_->LongTermAvgData());

  
  VCMResolutionScale* qm = NULL;
  int32_t ret = qm_resolution_->SelectResolution(&qm);
  if (ret < 0) {
    return ret;
  }

  
  QMUpdate(qm);

  
  qm_resolution_->ResetRates();

  
  last_qm_update_time_ = clock_->TimeInMilliseconds();

  
  content_->Reset();

  return VCM_OK;
}



int MediaOptimization::UpdateProtectionCallback(
    VCMProtectionMethod* selected_method,
    uint32_t* video_rate_bps,
    uint32_t* nack_overhead_rate_bps,
    uint32_t* fec_overhead_rate_bps) {
  if (!video_protection_callback_) {
    return VCM_OK;
  }
  FecProtectionParams delta_fec_params;
  FecProtectionParams key_fec_params;
  
  key_fec_params.fec_rate = selected_method->RequiredProtectionFactorK();

  
  delta_fec_params.fec_rate = selected_method->RequiredProtectionFactorD();

  
  key_fec_params.use_uep_protection = selected_method->RequiredUepProtectionK();

  
  delta_fec_params.use_uep_protection =
      selected_method->RequiredUepProtectionD();

  
  
  delta_fec_params.max_fec_frames = selected_method->MaxFramesFec();
  key_fec_params.max_fec_frames = selected_method->MaxFramesFec();

  
  
  
  
  delta_fec_params.fec_mask_type = kFecMaskRandom;
  key_fec_params.fec_mask_type = kFecMaskRandom;

  
  return video_protection_callback_->ProtectionRequest(&delta_fec_params,
                                                       &key_fec_params,
                                                       video_rate_bps,
                                                       nack_overhead_rate_bps,
                                                       fec_overhead_rate_bps);
}

void MediaOptimization::PurgeOldFrameSamples(int64_t now_ms) {
  while (!encoded_frame_samples_.empty()) {
    if (now_ms - encoded_frame_samples_.front().time_complete_ms >
        kBitrateAverageWinMs) {
      encoded_frame_samples_.pop_front();
    } else {
      break;
    }
  }
}

void MediaOptimization::UpdateSentBitrate(int64_t now_ms) {
  if (encoded_frame_samples_.empty()) {
    avg_sent_bit_rate_bps_ = 0;
    return;
  }
  int framesize_sum = 0;
  for (FrameSampleList::iterator it = encoded_frame_samples_.begin();
       it != encoded_frame_samples_.end();
       ++it) {
    framesize_sum += it->size_bytes;
  }
  float denom = static_cast<float>(
      now_ms - encoded_frame_samples_.front().time_complete_ms);
  if (denom >= 1.0f) {
    avg_sent_bit_rate_bps_ =
        static_cast<uint32_t>(framesize_sum * 8 * 1000 / denom + 0.5f);
  } else {
    avg_sent_bit_rate_bps_ = framesize_sum * 8;
  }
}

void MediaOptimization::UpdateSentFramerate() {
  if (encoded_frame_samples_.size() <= 1) {
    avg_sent_framerate_ = encoded_frame_samples_.size();
    return;
  }
  int denom = encoded_frame_samples_.back().timestamp -
              encoded_frame_samples_.front().timestamp;
  if (denom > 0) {
    avg_sent_framerate_ =
        (90000 * (encoded_frame_samples_.size() - 1) + denom / 2) / denom;
  } else {
    avg_sent_framerate_ = encoded_frame_samples_.size();
  }
}

bool MediaOptimization::QMUpdate(VCMResolutionScale* qm) {
  
  if (!qm->change_resolution_spatial && !qm->change_resolution_temporal) {
    return false;
  }

  
  if (qm->change_resolution_temporal) {
    incoming_frame_rate_ = qm->frame_rate;
    
    memset(incoming_frame_times_, -1, sizeof(incoming_frame_times_));
  }

  
  if (qm->change_resolution_spatial) {
    codec_width_ = qm->codec_width;
    codec_height_ = qm->codec_height;
  }

  WEBRTC_TRACE(webrtc::kTraceDebug,
               webrtc::kTraceVideoCoding,
               id_,
               "Resolution change from QM select: W = %d, H = %d, FR = %f",
               qm->codec_width,
               qm->codec_height,
               qm->frame_rate);

  
  
  
  
  
  
  video_qmsettings_callback_->SetVideoQMSettings(
      qm->frame_rate, codec_width_, codec_height_);
  content_->UpdateFrameRate(qm->frame_rate);
  qm_resolution_->UpdateCodecParameters(
      qm->frame_rate, codec_width_, codec_height_);
  return true;
}




bool MediaOptimization::CheckStatusForQMchange() {
  bool status = true;

  
  
  
  
  int64_t now = clock_->TimeInMilliseconds();
  if ((now - last_qm_update_time_) < kQmMinIntervalMs ||
      (now - last_change_time_) < kQmMinIntervalMs) {
    status = false;
  }

  return status;
}


void MediaOptimization::ProcessIncomingFrameRate(int64_t now) {
  int32_t num = 0;
  int32_t nr_of_frames = 0;
  for (num = 1; num < (kFrameCountHistorySize - 1); ++num) {
    if (incoming_frame_times_[num] <= 0 ||
        
        now - incoming_frame_times_[num] > kFrameHistoryWinMs) {
      break;
    } else {
      nr_of_frames++;
    }
  }
  if (num > 1) {
    const int64_t diff = now - incoming_frame_times_[num - 1];
    incoming_frame_rate_ = 1.0;
    if (diff > 0) {
      incoming_frame_rate_ = nr_of_frames * 1000.0f / static_cast<float>(diff);
    }
  }
}

}  
}  
