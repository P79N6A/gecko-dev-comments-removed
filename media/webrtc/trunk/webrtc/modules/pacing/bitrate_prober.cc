









#include "webrtc/modules/pacing/bitrate_prober.h"

#include <assert.h>
#include <limits>
#include <sstream>

#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {

namespace {
int ComputeDeltaFromBitrate(size_t packet_size, int bitrate_bps) {
  assert(bitrate_bps > 0);
  
  
  return static_cast<int>(1000ll * static_cast<int64_t>(packet_size) * 8ll /
      bitrate_bps);
}
}  

BitrateProber::BitrateProber()
    : probing_state_(kDisabled),
      packet_size_last_send_(0),
      time_last_send_ms_(-1) {
}

void BitrateProber::SetEnabled(bool enable) {
  if (enable) {
    if (probing_state_ == kDisabled) {
      probing_state_ = kAllowedToProbe;
      LOG(LS_INFO) << "Initial bandwidth probing enabled";
    }
  } else {
    probing_state_ = kDisabled;
    LOG(LS_INFO) << "Initial bandwidth probing disabled";
  }
}

bool BitrateProber::IsProbing() const {
  return probing_state_ == kProbing;
}

void BitrateProber::MaybeInitializeProbe(int bitrate_bps) {
  if (probing_state_ != kAllowedToProbe)
    return;
  probe_bitrates_.clear();
  
  const int kMaxNumProbes = 2;
  const int kPacketsPerProbe = 5;
  const float kProbeBitrateMultipliers[kMaxNumProbes] = {3, 6};
  int bitrates_bps[kMaxNumProbes];
  std::stringstream bitrate_log;
  bitrate_log << "Start probing for bandwidth, bitrates:";
  for (int i = 0; i < kMaxNumProbes; ++i) {
    bitrates_bps[i] = kProbeBitrateMultipliers[i] * bitrate_bps;
    bitrate_log << " " << bitrates_bps[i];
    
    if (i == 0)
      probe_bitrates_.push_back(bitrates_bps[i]);
    for (int j = 0; j < kPacketsPerProbe; ++j)
      probe_bitrates_.push_back(bitrates_bps[i]);
  }
  bitrate_log << ", num packets: " << probe_bitrates_.size();
  LOG(LS_INFO) << bitrate_log.str().c_str();
  probing_state_ = kProbing;
}

int BitrateProber::TimeUntilNextProbe(int64_t now_ms) {
  if (probing_state_ != kDisabled && probe_bitrates_.empty()) {
    probing_state_ = kWait;
  }
  if (probe_bitrates_.empty()) {
    
    return std::numeric_limits<int>::max();
  }
  int64_t elapsed_time_ms = now_ms - time_last_send_ms_;
  
  
  int time_until_probe_ms = 0;
  if (packet_size_last_send_ > 0 && probing_state_ == kProbing) {
    int next_delta_ms = ComputeDeltaFromBitrate(packet_size_last_send_,
                                                probe_bitrates_.front());
    time_until_probe_ms = next_delta_ms - elapsed_time_ms;
    
    
    const int kMinProbeDeltaMs = 1;
    
    
    const int kMaxProbeDelayMs = 3;
    if (next_delta_ms < kMinProbeDeltaMs ||
        time_until_probe_ms < -kMaxProbeDelayMs) {
      
      
      
      probing_state_ = kWait;
      LOG(LS_INFO) << "Next delta too small, stop probing.";
      time_until_probe_ms = 0;
    }
  }
  return time_until_probe_ms;
}

void BitrateProber::PacketSent(int64_t now_ms, size_t packet_size) {
  assert(packet_size > 0);
  packet_size_last_send_ = packet_size;
  time_last_send_ms_ = now_ms;
  if (probing_state_ != kProbing)
    return;
  if (!probe_bitrates_.empty())
    probe_bitrates_.pop_front();
}
}  
