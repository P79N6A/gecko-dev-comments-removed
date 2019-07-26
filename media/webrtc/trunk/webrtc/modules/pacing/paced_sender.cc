









#include "webrtc/modules/pacing/include/paced_sender.h"

#include <assert.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

namespace {




const float kBytesPerIntervalMargin = 1.5f;


const int kMinPacketLimitMs = 5;



const int kMaxIntervalTimeMs = 30;




const int kMaxQueueTimeWithoutSendingMs = 30;
}  

namespace webrtc {

PacedSender::PacedSender(Callback* callback, int target_bitrate_kbps)
    : callback_(callback),
      enable_(false),
      critsect_(CriticalSectionWrapper::CreateCriticalSection()),
      target_bitrate_kbytes_per_s_(target_bitrate_kbps >> 3),  
      bytes_remaining_interval_(0),
      padding_bytes_remaining_interval_(0),
      time_last_update_(TickTime::Now()) {
  UpdateBytesPerInterval(kMinPacketLimitMs);
}

PacedSender::~PacedSender() {
  normal_priority_packets_.clear();
  low_priority_packets_.clear();
}

void PacedSender::SetStatus(bool enable) {
  CriticalSectionScoped cs(critsect_.get());
  enable_ = enable;
}

void PacedSender::UpdateBitrate(int target_bitrate_kbps) {
  CriticalSectionScoped cs(critsect_.get());
  target_bitrate_kbytes_per_s_ = target_bitrate_kbps >> 3;  
}

bool PacedSender::SendPacket(Priority priority, uint32_t ssrc,
    uint16_t sequence_number, int64_t capture_time_ms, int bytes) {
  CriticalSectionScoped cs(critsect_.get());

  if (!enable_) {
    UpdateState(bytes);
    return true;  
  }
  switch (priority) {
    case kHighPriority:
      UpdateState(bytes);
      return true;  
    case kNormalPriority:
      if (normal_priority_packets_.empty() && bytes_remaining_interval_ > 0) {
        UpdateState(bytes);
        return true;  
      }
      normal_priority_packets_.push_back(
          Packet(ssrc, sequence_number, capture_time_ms, bytes));
      return false;
    case kLowPriority:
      if (normal_priority_packets_.empty() &&
          low_priority_packets_.empty() &&
          bytes_remaining_interval_ > 0) {
        UpdateState(bytes);
        return true;  
      }
      low_priority_packets_.push_back(
          Packet(ssrc, sequence_number, capture_time_ms, bytes));
      return false;
  }
  return false;
}

int32_t PacedSender::TimeUntilNextProcess() {
  CriticalSectionScoped cs(critsect_.get());
  int64_t elapsed_time_ms =
      (TickTime::Now() - time_last_update_).Milliseconds();
  if (elapsed_time_ms <= 0) {
    return kMinPacketLimitMs;
  }
  if (elapsed_time_ms >= kMinPacketLimitMs) {
    return 0;
  }
  return kMinPacketLimitMs - elapsed_time_ms;
}

int32_t PacedSender::Process() {
  TickTime now = TickTime::Now();
  CriticalSectionScoped cs(critsect_.get());
  int elapsed_time_ms = (now - time_last_update_).Milliseconds();
  time_last_update_ = now;
  if (elapsed_time_ms > 0) {
    uint32_t delta_time_ms = std::min(kMaxIntervalTimeMs, elapsed_time_ms);
    UpdateBytesPerInterval(delta_time_ms);
    uint32_t ssrc;
    uint16_t sequence_number;
    int64_t capture_time_ms;
    while (GetNextPacket(&ssrc, &sequence_number, &capture_time_ms)) {
      critsect_->Leave();
      callback_->TimeToSendPacket(ssrc, sequence_number, capture_time_ms);
      critsect_->Enter();
    }
    if (normal_priority_packets_.empty() &&
        low_priority_packets_.empty() &&
        padding_bytes_remaining_interval_ > 0) {
      critsect_->Leave();
      callback_->TimeToSendPadding(padding_bytes_remaining_interval_);
      critsect_->Enter();
      padding_bytes_remaining_interval_ = 0;
      bytes_remaining_interval_ -= padding_bytes_remaining_interval_;
    }
  }
  return 0;
}


void PacedSender::UpdateBytesPerInterval(uint32_t delta_time_ms) {
  uint32_t bytes_per_interval = target_bitrate_kbytes_per_s_ * delta_time_ms;

  if (bytes_remaining_interval_ < 0) {
    
    bytes_remaining_interval_ += kBytesPerIntervalMargin * bytes_per_interval;
  } else {
    
    bytes_remaining_interval_ = kBytesPerIntervalMargin * bytes_per_interval;
  }
  if (padding_bytes_remaining_interval_ < 0) {
    
    padding_bytes_remaining_interval_ += bytes_per_interval;
  } else {
    
    padding_bytes_remaining_interval_ = bytes_per_interval;
  }
}


bool PacedSender::GetNextPacket(uint32_t* ssrc, uint16_t* sequence_number,
                                int64_t* capture_time_ms) {
  if (bytes_remaining_interval_ <= 0) {
    
    
    if (!normal_priority_packets_.empty()) {
      if ((TickTime::Now() - time_last_send_).Milliseconds() >
          kMaxQueueTimeWithoutSendingMs) {
        Packet packet = normal_priority_packets_.front();
        UpdateState(packet.bytes_);
        *sequence_number = packet.sequence_number_;
        *ssrc = packet.ssrc_;
        *capture_time_ms = packet.capture_time_ms_;
        normal_priority_packets_.pop_front();
        return true;
      }
    }
    return false;
  }
  if (!normal_priority_packets_.empty()) {
    Packet packet = normal_priority_packets_.front();
    UpdateState(packet.bytes_);
    *sequence_number = packet.sequence_number_;
    *ssrc = packet.ssrc_;
    *capture_time_ms = packet.capture_time_ms_;
    normal_priority_packets_.pop_front();
    return true;
  }
  if (!low_priority_packets_.empty()) {
    Packet packet = low_priority_packets_.front();
    UpdateState(packet.bytes_);
    *sequence_number = packet.sequence_number_;
    *ssrc = packet.ssrc_;
    *capture_time_ms = packet.capture_time_ms_;
    low_priority_packets_.pop_front();
    return true;
  }
  return false;
}


void PacedSender::UpdateState(int num_bytes) {
  time_last_send_ = TickTime::Now();
  bytes_remaining_interval_ -= num_bytes;
  padding_bytes_remaining_interval_ -= num_bytes;
}

}  
