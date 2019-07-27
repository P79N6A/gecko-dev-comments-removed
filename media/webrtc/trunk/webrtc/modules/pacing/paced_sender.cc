









#include "webrtc/modules/pacing/include/paced_sender.h"

#include <assert.h>

#include <map>
#include <queue>
#include <set>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/pacing/bitrate_prober.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/field_trial.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace {

const int kMinPacketLimitMs = 5;



const int kMaxIntervalTimeMs = 30;

}  

namespace webrtc {
namespace paced_sender {
struct Packet {
  Packet(PacedSender::Priority priority,
         uint32_t ssrc,
         uint16_t seq_number,
         int64_t capture_time_ms,
         int64_t enqueue_time_ms,
         int length_in_bytes,
         bool retransmission,
         uint64_t enqueue_order)
      : priority(priority),
        ssrc(ssrc),
        sequence_number(seq_number),
        capture_time_ms(capture_time_ms),
        enqueue_time_ms(enqueue_time_ms),
        bytes(length_in_bytes),
        retransmission(retransmission),
        enqueue_order(enqueue_order) {}

  PacedSender::Priority priority;
  uint32_t ssrc;
  uint16_t sequence_number;
  int64_t capture_time_ms;
  int64_t enqueue_time_ms;
  int bytes;
  bool retransmission;
  uint64_t enqueue_order;
  std::list<Packet>::iterator this_it;
};


struct Comparator {
  bool operator()(const Packet* first, const Packet* second) {
    
    if (first->priority != second->priority)
      return first->priority > second->priority;

    
    if (second->retransmission && !first->retransmission)
      return true;

    
    if (first->capture_time_ms != second->capture_time_ms)
      return first->capture_time_ms > second->capture_time_ms;

    return first->enqueue_order > second->enqueue_order;
  }
};


class PacketQueue {
 public:
  PacketQueue() : bytes_(0) {}
  virtual ~PacketQueue() {}

  void Push(const Packet& packet) {
    if (!AddToDupeSet(packet))
      return;

    
    
    
    packet_list_.push_front(packet);
    std::list<Packet>::iterator it = packet_list_.begin();
    it->this_it = it;          
    prio_queue_.push(&(*it));  
    bytes_ += packet.bytes;
  }

  const Packet& BeginPop() {
    const Packet& packet = *prio_queue_.top();
    prio_queue_.pop();
    return packet;
  }

  void CancelPop(const Packet& packet) { prio_queue_.push(&(*packet.this_it)); }

  void FinalizePop(const Packet& packet) {
    RemoveFromDupeSet(packet);
    bytes_ -= packet.bytes;
    packet_list_.erase(packet.this_it);
  }

  bool Empty() const { return prio_queue_.empty(); }

  size_t SizeInPackets() const { return prio_queue_.size(); }

  uint32_t SizeInBytes() const { return bytes_; }

  int64_t OldestEnqueueTime() const {
    std::list<Packet>::const_reverse_iterator it = packet_list_.rbegin();
    if (it == packet_list_.rend())
      return 0;
    return it->enqueue_time_ms;
  }

 private:
  
  
  bool AddToDupeSet(const Packet& packet) {
    SsrcSeqNoMap::iterator it = dupe_map_.find(packet.ssrc);
    if (it == dupe_map_.end()) {
      
      dupe_map_[packet.ssrc].insert(packet.sequence_number);
      return true;
    }

    
    return it->second.insert(packet.sequence_number).second;
  }

  void RemoveFromDupeSet(const Packet& packet) {
    SsrcSeqNoMap::iterator it = dupe_map_.find(packet.ssrc);
    assert(it != dupe_map_.end());
    it->second.erase(packet.sequence_number);
    if (it->second.empty()) {
      dupe_map_.erase(it);
    }
  }

  
  
  std::list<Packet> packet_list_;
  
  
  std::priority_queue<Packet*, std::vector<Packet*>, Comparator> prio_queue_;
  
  uint64_t bytes_;
  
  typedef std::map<uint32_t, std::set<uint16_t> > SsrcSeqNoMap;
  SsrcSeqNoMap dupe_map_;
};

class IntervalBudget {
 public:
  explicit IntervalBudget(int initial_target_rate_kbps)
      : target_rate_kbps_(initial_target_rate_kbps),
        bytes_remaining_(0) {}

  void set_target_rate_kbps(int target_rate_kbps) {
    target_rate_kbps_ = target_rate_kbps;
  }

  void IncreaseBudget(int delta_time_ms) {
    int bytes = target_rate_kbps_ * delta_time_ms / 8;
    if (bytes_remaining_ < 0) {
      
      bytes_remaining_ = bytes_remaining_ + bytes;
    } else {
      
      bytes_remaining_ = bytes;
    }
  }

  void UseBudget(int bytes) {
    bytes_remaining_ = std::max(bytes_remaining_ - bytes,
                                -500 * target_rate_kbps_ / 8);
  }

  int bytes_remaining() const { return bytes_remaining_; }

  int target_rate_kbps() const { return target_rate_kbps_; }

 private:
  int target_rate_kbps_;
  int bytes_remaining_;
};
}  

const float PacedSender::kDefaultPaceMultiplier = 2.5f;

PacedSender::PacedSender(Clock* clock,
                         Callback* callback,
                         int bitrate_kbps,
                         int max_bitrate_kbps,
                         int min_bitrate_kbps)
    : clock_(clock),
      callback_(callback),
      critsect_(CriticalSectionWrapper::CreateCriticalSection()),
      enabled_(true),
      paused_(false),
      media_budget_(new paced_sender::IntervalBudget(max_bitrate_kbps)),
      padding_budget_(new paced_sender::IntervalBudget(min_bitrate_kbps)),
      prober_(new BitrateProber()),
      bitrate_bps_(1000 * bitrate_kbps),
      time_last_update_us_(clock->TimeInMicroseconds()),
      packets_(new paced_sender::PacketQueue()),
      packet_counter_(0) {
  UpdateBytesPerInterval(kMinPacketLimitMs);
}

PacedSender::~PacedSender() {}

void PacedSender::Pause() {
  CriticalSectionScoped cs(critsect_.get());
  paused_ = true;
}

void PacedSender::Resume() {
  CriticalSectionScoped cs(critsect_.get());
  paused_ = false;
}

void PacedSender::SetStatus(bool enable) {
  CriticalSectionScoped cs(critsect_.get());
  enabled_ = enable;
}

bool PacedSender::Enabled() const {
  CriticalSectionScoped cs(critsect_.get());
  return enabled_;
}

void PacedSender::UpdateBitrate(int bitrate_kbps,
                                int max_bitrate_kbps,
                                int min_bitrate_kbps) {
  CriticalSectionScoped cs(critsect_.get());
  media_budget_->set_target_rate_kbps(max_bitrate_kbps);
  padding_budget_->set_target_rate_kbps(min_bitrate_kbps);
  bitrate_bps_ = 1000 * bitrate_kbps;
}

bool PacedSender::SendPacket(Priority priority, uint32_t ssrc,
    uint16_t sequence_number, int64_t capture_time_ms, int bytes,
    bool retransmission) {
  CriticalSectionScoped cs(critsect_.get());

  if (!enabled_) {
    return true;  
  }
  
  if (!prober_->IsProbing() && ProbingExperimentIsEnabled()) {
    prober_->SetEnabled(true);
  }
  prober_->MaybeInitializeProbe(bitrate_bps_);

  if (capture_time_ms < 0) {
    capture_time_ms = clock_->TimeInMilliseconds();
  }

  packets_->Push(paced_sender::Packet(
      priority, ssrc, sequence_number, capture_time_ms,
      clock_->TimeInMilliseconds(), bytes, retransmission, packet_counter_++));
  return false;
}

int PacedSender::ExpectedQueueTimeMs() const {
  CriticalSectionScoped cs(critsect_.get());
  int target_rate = media_budget_->target_rate_kbps();
  assert(target_rate > 0);
  return packets_->SizeInBytes() * 8 / target_rate;
}

size_t PacedSender::QueueSizePackets() const {
  CriticalSectionScoped cs(critsect_.get());
  return packets_->SizeInPackets();
}

int PacedSender::QueueInMs() const {
  CriticalSectionScoped cs(critsect_.get());

  int64_t oldest_packet = packets_->OldestEnqueueTime();
  if (oldest_packet == 0)
    return 0;

  return clock_->TimeInMilliseconds() - oldest_packet;
}

int32_t PacedSender::TimeUntilNextProcess() {
  CriticalSectionScoped cs(critsect_.get());
  int64_t elapsed_time_us = clock_->TimeInMicroseconds() - time_last_update_us_;
  int elapsed_time_ms = static_cast<int>((elapsed_time_us + 500) / 1000);
  if (prober_->IsProbing()) {
    int next_probe = prober_->TimeUntilNextProbe(clock_->TimeInMilliseconds());
    return next_probe;
  }
  if (elapsed_time_ms >= kMinPacketLimitMs) {
    return 0;
  }
  return kMinPacketLimitMs - elapsed_time_ms;
}

int32_t PacedSender::Process() {
  int64_t now_us = clock_->TimeInMicroseconds();
  CriticalSectionScoped cs(critsect_.get());
  int elapsed_time_ms = (now_us - time_last_update_us_ + 500) / 1000;
  time_last_update_us_ = now_us;
  if (!enabled_) {
    return 0;
  }
  if (!paused_) {
    if (elapsed_time_ms > 0) {
      uint32_t delta_time_ms = std::min(kMaxIntervalTimeMs, elapsed_time_ms);
      UpdateBytesPerInterval(delta_time_ms);
    }

    while (!packets_->Empty()) {
      if (media_budget_->bytes_remaining() <= 0 && !prober_->IsProbing())
        return 0;

      
      
      
      const paced_sender::Packet& packet = packets_->BeginPop();
      if (SendPacket(packet)) {
        
        packets_->FinalizePop(packet);
        if (prober_->IsProbing())
          return 0;
      } else {
        
        packets_->CancelPop(packet);
        return 0;
      }
    }

    int padding_needed = padding_budget_->bytes_remaining();
    if (padding_needed > 0) {
      SendPadding(padding_needed);
    }
  }
  return 0;
}

bool PacedSender::SendPacket(const paced_sender::Packet& packet) {
  critsect_->Leave();
  const bool success = callback_->TimeToSendPacket(packet.ssrc,
                                                   packet.sequence_number,
                                                   packet.capture_time_ms,
                                                   packet.retransmission);
  critsect_->Enter();

  if (success) {
    
    prober_->PacketSent(clock_->TimeInMilliseconds(), packet.bytes);
    media_budget_->UseBudget(packet.bytes);
    padding_budget_->UseBudget(packet.bytes);
  }

  return success;
}

void PacedSender::SendPadding(int padding_needed) {
  critsect_->Leave();
  int bytes_sent = callback_->TimeToSendPadding(padding_needed);
  critsect_->Enter();

  
  media_budget_->UseBudget(bytes_sent);
  padding_budget_->UseBudget(bytes_sent);
}

void PacedSender::UpdateBytesPerInterval(uint32_t delta_time_ms) {
  media_budget_->IncreaseBudget(delta_time_ms);
  padding_budget_->IncreaseBudget(delta_time_ms);
}

bool PacedSender::ProbingExperimentIsEnabled() const {
#ifndef WEBRTC_MOZILLA_BUILD
  return webrtc::field_trial::FindFullName("WebRTC-BitrateProbing") ==
         "Enabled";
#else
  return false;
#endif
}
}  
