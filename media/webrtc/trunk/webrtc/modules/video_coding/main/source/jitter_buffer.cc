








#include "modules/video_coding/main/source/jitter_buffer.h"

#include <algorithm>
#include <cassert>

#include "modules/video_coding/main/source/event.h"
#include "modules/video_coding/main/source/frame_buffer.h"
#include "modules/video_coding/main/source/inter_frame_delay.h"
#include "modules/video_coding/main/source/internal_defines.h"
#include "modules/video_coding/main/source/jitter_buffer_common.h"
#include "modules/video_coding/main/source/jitter_estimator.h"
#include "modules/video_coding/main/source/packet.h"
#include "modules/video_coding/main/source/tick_time_base.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {


static uint32_t kDefaultRtt = 200;


class FrameSmallerTimestamp {
 public:
  explicit FrameSmallerTimestamp(uint32_t timestamp) : timestamp_(timestamp) {}
  bool operator()(VCMFrameBuffer* frame) {
    return (LatestTimestamp(timestamp_, frame->TimeStamp(), NULL) ==
            timestamp_);
  }

 private:
  uint32_t timestamp_;
};

class FrameEqualTimestamp {
 public:
  explicit FrameEqualTimestamp(uint32_t timestamp) : timestamp_(timestamp) {}
  bool operator()(VCMFrameBuffer* frame) {
    return (timestamp_ == frame->TimeStamp());
  }

 private:
  uint32_t timestamp_;
};

class CompleteDecodableKeyFrameCriteria {
 public:
  bool operator()(VCMFrameBuffer* frame) {
    return (frame->FrameType() == kVideoFrameKey) &&
           (frame->GetState() == kStateComplete ||
            frame->GetState() == kStateDecodable);
  }
};

VCMJitterBuffer::VCMJitterBuffer(TickTimeBase* clock,
                                 int vcm_id,
                                 int receiver_id,
                                 bool master)
    : vcm_id_(vcm_id),
      receiver_id_(receiver_id),
      clock_(clock),
      running_(false),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      master_(master),
      frame_event_(),
      packet_event_(),
      max_number_of_frames_(kStartNumberOfFrames),
      frame_buffers_(),
      frame_list_(),
      last_decoded_state_(),
      first_packet_(true),
      num_not_decodable_packets_(0),
      receive_statistics_(),
      incoming_frame_rate_(0),
      incoming_frame_count_(0),
      time_last_incoming_frame_count_(0),
      incoming_bit_count_(0),
      incoming_bit_rate_(0),
      drop_count_(0),
      num_consecutive_old_frames_(0),
      num_consecutive_old_packets_(0),
      num_discarded_packets_(0),
      jitter_estimate_(vcm_id, receiver_id),
      inter_frame_delay_(clock_->MillisecondTimestamp()),
      rtt_ms_(kDefaultRtt),
      nack_mode_(kNoNack),
      low_rtt_nack_threshold_ms_(-1),
      high_rtt_nack_threshold_ms_(-1),
      nack_seq_nums_(),
      nack_seq_nums_length_(0),
      waiting_for_key_frame_(false) {
  memset(frame_buffers_, 0, sizeof(frame_buffers_));
  memset(receive_statistics_, 0, sizeof(receive_statistics_));
  memset(nack_seq_nums_internal_, -1, sizeof(nack_seq_nums_internal_));

  for (int i = 0; i < kStartNumberOfFrames; i++) {
    frame_buffers_[i] = new VCMFrameBuffer();
  }
}

VCMJitterBuffer::~VCMJitterBuffer() {
  Stop();
  for (int i = 0; i < kMaxNumberOfFrames; i++) {
    if (frame_buffers_[i]) {
      delete frame_buffers_[i];
    }
  }
  delete crit_sect_;
}

void VCMJitterBuffer::CopyFrom(const VCMJitterBuffer& rhs) {
  if (this != &rhs) {
    crit_sect_->Enter();
    rhs.crit_sect_->Enter();
    vcm_id_ = rhs.vcm_id_;
    receiver_id_ = rhs.receiver_id_;
    running_ = rhs.running_;
    master_ = !rhs.master_;
    max_number_of_frames_ = rhs.max_number_of_frames_;
    incoming_frame_rate_ = rhs.incoming_frame_rate_;
    incoming_frame_count_ = rhs.incoming_frame_count_;
    time_last_incoming_frame_count_ = rhs.time_last_incoming_frame_count_;
    incoming_bit_count_ = rhs.incoming_bit_count_;
    incoming_bit_rate_ = rhs.incoming_bit_rate_;
    drop_count_ = rhs.drop_count_;
    num_consecutive_old_frames_ = rhs.num_consecutive_old_frames_;
    num_consecutive_old_packets_ = rhs.num_consecutive_old_packets_;
    num_discarded_packets_ = rhs.num_discarded_packets_;
    jitter_estimate_ = rhs.jitter_estimate_;
    inter_frame_delay_ = rhs.inter_frame_delay_;
    waiting_for_completion_ = rhs.waiting_for_completion_;
    rtt_ms_ = rhs.rtt_ms_;
    nack_seq_nums_length_ = rhs.nack_seq_nums_length_;
    waiting_for_key_frame_ = rhs.waiting_for_key_frame_;
    first_packet_ = rhs.first_packet_;
    last_decoded_state_ =  rhs.last_decoded_state_;
    num_not_decodable_packets_ = rhs.num_not_decodable_packets_;
    memcpy(receive_statistics_, rhs.receive_statistics_,
           sizeof(receive_statistics_));
    memcpy(nack_seq_nums_internal_, rhs.nack_seq_nums_internal_,
           sizeof(nack_seq_nums_internal_));
    memcpy(nack_seq_nums_, rhs.nack_seq_nums_, sizeof(nack_seq_nums_));
    for (int i = 0; i < kMaxNumberOfFrames; i++) {
      if (frame_buffers_[i] != NULL) {
        delete frame_buffers_[i];
        frame_buffers_[i] = NULL;
      }
    }
    frame_list_.clear();
    for (int i = 0; i < max_number_of_frames_; i++) {
      frame_buffers_[i] = new VCMFrameBuffer(*(rhs.frame_buffers_[i]));
      if (frame_buffers_[i]->Length() > 0) {
        FrameList::reverse_iterator rit = std::find_if(
            frame_list_.rbegin(), frame_list_.rend(),
            FrameSmallerTimestamp(frame_buffers_[i]->TimeStamp()));
        frame_list_.insert(rit.base(), frame_buffers_[i]);
      }
    }
    rhs.crit_sect_->Leave();
    crit_sect_->Leave();
  }
}

void VCMJitterBuffer::Start() {
  CriticalSectionScoped cs(crit_sect_);
  running_ = true;
  incoming_frame_count_ = 0;
  incoming_frame_rate_ = 0;
  incoming_bit_count_ = 0;
  incoming_bit_rate_ = 0;
  time_last_incoming_frame_count_ = clock_->MillisecondTimestamp();
  memset(receive_statistics_, 0, sizeof(receive_statistics_));

  num_consecutive_old_frames_ = 0;
  num_consecutive_old_packets_ = 0;
  num_discarded_packets_ = 0;

  
  frame_event_.Reset();
  packet_event_.Reset();
  waiting_for_completion_.frame_size = 0;
  waiting_for_completion_.timestamp = 0;
  waiting_for_completion_.latest_packet_time = -1;
  first_packet_ = true;
  nack_seq_nums_length_ = 0;
  waiting_for_key_frame_ = false;
  rtt_ms_ = kDefaultRtt;
  num_not_decodable_packets_ = 0;

  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
               VCMId(vcm_id_, receiver_id_), "JB(0x%x): Jitter buffer: start",
               this);
}

void VCMJitterBuffer::Stop() {
  crit_sect_->Enter();
  running_ = false;
  last_decoded_state_.Reset();
  frame_list_.clear();
  for (int i = 0; i < kMaxNumberOfFrames; i++) {
    if (frame_buffers_[i] != NULL) {
      static_cast<VCMFrameBuffer*>(frame_buffers_[i])->SetState(kStateFree);
    }
  }

  crit_sect_->Leave();
  
  frame_event_.Set();
  packet_event_.Set();
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
               VCMId(vcm_id_, receiver_id_), "JB(0x%x): Jitter buffer: stop",
               this);
}

bool VCMJitterBuffer::Running() const {
  CriticalSectionScoped cs(crit_sect_);
  return running_;
}

void VCMJitterBuffer::Flush() {
  CriticalSectionScoped cs(crit_sect_);
  
  frame_list_.clear();
  for (int i = 0; i < max_number_of_frames_; i++) {
    ReleaseFrameIfNotDecoding(frame_buffers_[i]);
  }
  last_decoded_state_.Reset();  
  num_not_decodable_packets_ = 0;
  frame_event_.Reset();
  packet_event_.Reset();
  num_consecutive_old_frames_ = 0;
  num_consecutive_old_packets_ = 0;
  
  jitter_estimate_.Reset();
  inter_frame_delay_.Reset(clock_->MillisecondTimestamp());
  waiting_for_completion_.frame_size = 0;
  waiting_for_completion_.timestamp = 0;
  waiting_for_completion_.latest_packet_time = -1;
  first_packet_ = true;
  nack_seq_nums_length_ = 0;
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
               VCMId(vcm_id_, receiver_id_), "JB(0x%x): Jitter buffer: flush",
               this);
}


void VCMJitterBuffer::FrameStatistics(uint32_t* received_delta_frames,
                                      uint32_t* received_key_frames) const {
  assert(received_delta_frames);
  assert(received_key_frames);
  CriticalSectionScoped cs(crit_sect_);
  *received_delta_frames = receive_statistics_[1] + receive_statistics_[3];
  *received_key_frames = receive_statistics_[0] + receive_statistics_[2];
}

int VCMJitterBuffer::num_not_decodable_packets() const {
  CriticalSectionScoped cs(crit_sect_);
  return num_not_decodable_packets_;
}

int VCMJitterBuffer::num_discarded_packets() const {
  CriticalSectionScoped cs(crit_sect_);
  return num_discarded_packets_;
}


void VCMJitterBuffer::IncomingRateStatistics(unsigned int* framerate,
                                             unsigned int* bitrate) {
  assert(framerate);
  assert(bitrate);
  CriticalSectionScoped cs(crit_sect_);
  const int64_t now = clock_->MillisecondTimestamp();
  int64_t diff = now - time_last_incoming_frame_count_;
  if (diff < 1000 && incoming_frame_rate_ > 0 && incoming_bit_rate_ > 0) {
    
    
    *framerate = incoming_frame_rate_;
    *bitrate = incoming_bit_rate_;
  } else if (incoming_frame_count_ != 0) {
    

    
    if (diff <= 0) {
      diff = 1;
    }
    
    float rate = 0.5f + ((incoming_frame_count_ * 1000.0f) / diff);
    if (rate < 1.0f) {
      rate = 1.0f;
    }

    
    
    
    
    
    
    *framerate = (incoming_frame_rate_ + static_cast<unsigned int>(rate)) / 2;
    incoming_frame_rate_ = static_cast<unsigned int>(rate);

    
    if (incoming_bit_count_ == 0) {
      *bitrate = 0;
    } else {
      *bitrate = 10 * ((100 * incoming_bit_count_) /
                       static_cast<unsigned int>(diff));
    }
    incoming_bit_rate_ = *bitrate;

    
    incoming_frame_count_ = 0;
    incoming_bit_count_ = 0;
    time_last_incoming_frame_count_ = now;

  } else {
    
    time_last_incoming_frame_count_ = clock_->MillisecondTimestamp();
    *framerate = 0;
    bitrate = 0;
    incoming_bit_rate_ = 0;
  }
}


int64_t VCMJitterBuffer::NextTimestamp(uint32_t max_wait_time_ms,
                                       FrameType* incoming_frame_type,
                                       int64_t* render_time_ms) {
  assert(incoming_frame_type);
  assert(render_time_ms);
  if (!running_) {
    return -1;
  }

  crit_sect_->Enter();

  
  CleanUpOldFrames();

  FrameList::iterator it = frame_list_.begin();

  if (it == frame_list_.end()) {
    packet_event_.Reset();
    crit_sect_->Leave();

    if (packet_event_.Wait(max_wait_time_ms) == kEventSignaled) {
      
      if (!running_) {
        return -1;
      }
      crit_sect_->Enter();

      CleanUpOldFrames();
      it = frame_list_.begin();
    } else {
      crit_sect_->Enter();
    }
  }

  if (it == frame_list_.end()) {
    crit_sect_->Leave();
    return -1;
  }
  
  *incoming_frame_type = (*it)->FrameType();
  *render_time_ms = (*it)->RenderTimeMs();
  const uint32_t timestamp = (*it)->TimeStamp();
  crit_sect_->Leave();

  return timestamp;
}






bool VCMJitterBuffer::CompleteSequenceWithNextFrame() {
  CriticalSectionScoped cs(crit_sect_);
  
  CleanUpOldFrames();

  if (frame_list_.empty())
    return true;

  VCMFrameBuffer* oldest_frame = frame_list_.front();
  if (frame_list_.size() <= 1 &&
      oldest_frame->GetState() != kStateComplete) {
    
    return true;
  }
  if (!oldest_frame->Complete()) {
    return false;
  }

  
  if (last_decoded_state_.init()) {
    
    if (oldest_frame->FrameType() != kVideoFrameKey) {
      return false;
    }
  } else if (oldest_frame->GetLowSeqNum() == -1) {
    return false;
  } else if (!last_decoded_state_.ContinuousFrame(oldest_frame)) {
    return false;
  }
  return true;
}



VCMEncodedFrame* VCMJitterBuffer::GetCompleteFrameForDecoding(
    uint32_t max_wait_time_ms) {
  if (!running_) {
    return NULL;
  }

  crit_sect_->Enter();

  CleanUpOldFrames();

  if (last_decoded_state_.init() && WaitForRetransmissions()) {
    waiting_for_key_frame_ = true;
  }

  FrameList::iterator it = FindOldestCompleteContinuousFrame(false);
  if (it == frame_list_.end()) {
    if (max_wait_time_ms == 0) {
      crit_sect_->Leave();
      return NULL;
    }
    const int64_t end_wait_time_ms = clock_->MillisecondTimestamp()
                                           + max_wait_time_ms;
    int64_t wait_time_ms = max_wait_time_ms;
    while (wait_time_ms > 0) {
      crit_sect_->Leave();
      const EventTypeWrapper ret =
        frame_event_.Wait(static_cast<uint32_t>(wait_time_ms));
      crit_sect_->Enter();
      if (ret == kEventSignaled) {
        
        if (!running_) {
          crit_sect_->Leave();
          return NULL;
        }

        
        
        CleanUpOldFrames();
        it = FindOldestCompleteContinuousFrame(false);
        if (it == frame_list_.end()) {
          wait_time_ms = end_wait_time_ms -
                         clock_->MillisecondTimestamp();
        } else {
          break;
        }
      } else {
        crit_sect_->Leave();
        return NULL;
      }
    }
    
  } else {
    
    frame_event_.Reset();
  }

  if (it == frame_list_.end()) {
    
    crit_sect_->Leave();
    return NULL;
  }

  VCMFrameBuffer* oldest_frame = *it;
  it = frame_list_.erase(it);

  
  const bool retransmitted = (oldest_frame->GetNackCount() > 0);
  if (retransmitted) {
    jitter_estimate_.FrameNacked();
  } else if (oldest_frame->Length() > 0) {
    
    UpdateJitterEstimate(*oldest_frame, false);
  }

  oldest_frame->SetState(kStateDecoding);

  CleanUpOldFrames();

  if (oldest_frame->FrameType() == kVideoFrameKey) {
    waiting_for_key_frame_ = false;
  }

  
  last_decoded_state_.SetState(oldest_frame);

  crit_sect_->Leave();

  return oldest_frame;
}

VCMEncodedFrame* VCMJitterBuffer::GetFrameForDecoding() {
  CriticalSectionScoped cs(crit_sect_);
  if (!running_) {
    return NULL;
  }

  if (WaitForRetransmissions()) {
    return GetFrameForDecodingNACK();
  }

  CleanUpOldFrames();

  if (frame_list_.empty()) {
    return NULL;
  }

  VCMFrameBuffer* oldest_frame = frame_list_.front();
  if (frame_list_.size() <= 1 &&
      oldest_frame->GetState() != kStateComplete) {
    return NULL;
  }

  
  
  
  
  const bool retransmitted = (oldest_frame->GetNackCount() > 0);
  if (retransmitted) {
    jitter_estimate_.FrameNacked();
  } else if (oldest_frame->Length() > 0) {
    
    
    if (waiting_for_completion_.latest_packet_time >= 0) {
      UpdateJitterEstimate(waiting_for_completion_, true);
    }
    
    waiting_for_completion_.frame_size = oldest_frame->Length();
    waiting_for_completion_.latest_packet_time =
      oldest_frame->LatestPacketTimeMs();
    waiting_for_completion_.timestamp = oldest_frame->TimeStamp();
  }
  frame_list_.erase(frame_list_.begin());

  
  VerifyAndSetPreviousFrameLost(oldest_frame);

  
  
  
  
  oldest_frame->SetState(kStateDecoding);

  CleanUpOldFrames();

  if (oldest_frame->FrameType() == kVideoFrameKey) {
    waiting_for_key_frame_ = false;
  }

  num_not_decodable_packets_ += oldest_frame->NotDecodablePackets();

  
  last_decoded_state_.SetState(oldest_frame);

  return oldest_frame;
}



void VCMJitterBuffer::ReleaseFrame(VCMEncodedFrame* frame) {
  CriticalSectionScoped cs(crit_sect_);
  VCMFrameBuffer* frame_buffer = static_cast<VCMFrameBuffer*>(frame);
  if (frame_buffer)
    frame_buffer->SetState(kStateFree);
}


int VCMJitterBuffer::GetFrame(const VCMPacket& packet,
                               VCMEncodedFrame*& frame) {
  if (!running_) {  
    return VCM_UNINITIALIZED;
  }

  crit_sect_->Enter();
  
  if (last_decoded_state_.IsOldPacket(&packet)) {
    
    if (packet.sizeBytes > 0) {
      num_discarded_packets_++;
      num_consecutive_old_packets_++;
    }
    
    
    
    last_decoded_state_.UpdateOldPacket(&packet);

    if (num_consecutive_old_packets_ > kMaxConsecutiveOldPackets) {
      Flush();
      crit_sect_->Leave();
      return VCM_FLUSH_INDICATOR;
    }
    crit_sect_->Leave();
    return VCM_OLD_PACKET_ERROR;
  }
  num_consecutive_old_packets_ = 0;

  FrameList::iterator it = std::find_if(
                             frame_list_.begin(),
                             frame_list_.end(),
                             FrameEqualTimestamp(packet.timestamp));

  if (it != frame_list_.end()) {
    frame = *it;
    crit_sect_->Leave();
    return VCM_OK;
  }

  crit_sect_->Leave();

  
  frame = GetEmptyFrame();
  if (frame != NULL) {
    return VCM_OK;
  }
  
  crit_sect_->Enter();
  RecycleFramesUntilKeyFrame();
  crit_sect_->Leave();

  frame = GetEmptyFrame();
  if (frame != NULL) {
    return VCM_OK;
  }
  return VCM_JITTER_BUFFER_ERROR;
}


VCMEncodedFrame* VCMJitterBuffer::GetFrame(const VCMPacket& packet) {
  VCMEncodedFrame* frame = NULL;
  if (GetFrame(packet, frame) < 0) {
    return NULL;
  }
  return frame;
}

int64_t VCMJitterBuffer::LastPacketTime(VCMEncodedFrame* frame,
                                        bool* retransmitted) const {
  assert(retransmitted);
  CriticalSectionScoped cs(crit_sect_);
  *retransmitted = (static_cast<VCMFrameBuffer*>(frame)->GetNackCount() > 0);
  return static_cast<VCMFrameBuffer*>(frame)->LatestPacketTimeMs();
}

VCMFrameBufferEnum VCMJitterBuffer::InsertPacket(VCMEncodedFrame* encoded_frame,
                                                 const VCMPacket& packet) {
  assert(encoded_frame);
  CriticalSectionScoped cs(crit_sect_);
  int64_t now_ms = clock_->MillisecondTimestamp();
  VCMFrameBufferEnum buffer_return = kSizeError;
  VCMFrameBufferEnum ret = kSizeError;
  VCMFrameBuffer* frame = static_cast<VCMFrameBuffer*>(encoded_frame);

  
  
  if (first_packet_) {
    
    
    inter_frame_delay_.Reset(clock_->MillisecondTimestamp());
    first_packet_ = false;
  }

  
  
  if (packet.frameType != kFrameEmpty) {
    if (waiting_for_completion_.timestamp == packet.timestamp) {
      
      
      waiting_for_completion_.frame_size += packet.sizeBytes;
      waiting_for_completion_.latest_packet_time = now_ms;
    } else if (waiting_for_completion_.latest_packet_time >= 0 &&
               waiting_for_completion_.latest_packet_time + 2000 <= now_ms) {
      
      UpdateJitterEstimate(waiting_for_completion_, true);
      waiting_for_completion_.latest_packet_time = -1;
      waiting_for_completion_.frame_size = 0;
      waiting_for_completion_.timestamp = 0;
    }
  }

  VCMFrameBufferStateEnum state = frame->GetState();
  last_decoded_state_.UpdateOldPacket(&packet);
  
  
  
  
  bool first = (frame->GetHighSeqNum() == -1);
  
  
  
  
  buffer_return = frame->InsertPacket(packet, now_ms,
                                      nack_mode_ == kNackHybrid,
                                      rtt_ms_);
  ret = buffer_return;
  if (buffer_return > 0) {
    incoming_bit_count_ += packet.sizeBytes << 3;

    
    if (IsPacketRetransmitted(packet)) {
      frame->IncrementNackCount();
    }

    
    
    if (state == kStateEmpty && first) {
      ret = kFirstPacket;
      FrameList::reverse_iterator rit = std::find_if(
          frame_list_.rbegin(),
          frame_list_.rend(),
          FrameSmallerTimestamp(frame->TimeStamp()));
      frame_list_.insert(rit.base(), frame);
    }
  }
  switch (buffer_return) {
    case kStateError:
    case kTimeStampError:
    case kSizeError: {
      if (frame != NULL) {
        
        frame->Reset();
        frame->SetState(kStateEmpty);
      }
      break;
    }
    case kCompleteSession: {
      
      if (UpdateFrameState(frame) == kFlushIndicator)
        ret = kFlushIndicator;
      
      packet_event_.Set();
      break;
    }
    case kDecodableSession:
    case kIncomplete: {
      
      packet_event_.Set();
      break;
    }
    case kNoError:
    case kDuplicatePacket: {
      break;
    }
    default: {
      assert(false && "JitterBuffer::InsertPacket: Undefined value");
    }
  }
  return ret;
}

uint32_t VCMJitterBuffer::EstimatedJitterMs() {
  CriticalSectionScoped cs(crit_sect_);
  uint32_t estimate = VCMJitterEstimator::OPERATING_SYSTEM_JITTER;

  
  
  double rtt_mult = 1.0f;
  if (nack_mode_ == kNackHybrid && (low_rtt_nack_threshold_ms_ >= 0 &&
      static_cast<int>(rtt_ms_) > low_rtt_nack_threshold_ms_)) {
    
    rtt_mult = 0.0f;
  }
  estimate += static_cast<uint32_t>
              (jitter_estimate_.GetJitterEstimate(rtt_mult) + 0.5);
  return estimate;
}

void VCMJitterBuffer::UpdateRtt(uint32_t rtt_ms) {
  CriticalSectionScoped cs(crit_sect_);
  rtt_ms_ = rtt_ms;
  jitter_estimate_.UpdateRtt(rtt_ms);
}

void VCMJitterBuffer::SetNackMode(VCMNackMode mode,
                                  int low_rtt_nack_threshold_ms,
                                  int high_rtt_nack_threshold_ms) {
  CriticalSectionScoped cs(crit_sect_);
  nack_mode_ = mode;
  assert(low_rtt_nack_threshold_ms >= -1 && high_rtt_nack_threshold_ms >= -1);
  assert(high_rtt_nack_threshold_ms == -1 ||
         low_rtt_nack_threshold_ms <= high_rtt_nack_threshold_ms);
  assert(low_rtt_nack_threshold_ms > -1 || high_rtt_nack_threshold_ms == -1);
  low_rtt_nack_threshold_ms_ = low_rtt_nack_threshold_ms;
  high_rtt_nack_threshold_ms_ = high_rtt_nack_threshold_ms;
  
  
  if (rtt_ms_ == kDefaultRtt && high_rtt_nack_threshold_ms_ != -1) {
    rtt_ms_ = 0;
  }
  if (nack_mode_ == kNoNack) {
    jitter_estimate_.ResetNackCount();
  }
}

VCMNackMode VCMJitterBuffer::nack_mode() const {
  CriticalSectionScoped cs(crit_sect_);
  return nack_mode_;
}

uint16_t* VCMJitterBuffer::CreateNackList(uint16_t* nack_list_size,
                                          bool* list_extended) {
  assert(nack_list_size);
  assert(list_extended);
  
  CriticalSectionScoped cs(crit_sect_);
  int i = 0;
  int32_t low_seq_num = -1;
  int32_t high_seq_num = -1;
  *list_extended = false;

  
  if (!WaitForRetransmissions()) {
    *nack_list_size = 0;
    return NULL;
  }

  
  
  
  
  GetLowHighSequenceNumbers(&low_seq_num, &high_seq_num);

  
  if (low_seq_num == -1 || high_seq_num == -1) {
    
    if (high_seq_num == -1) {
      
      *nack_list_size = 0;
    } else {
      
      *nack_list_size = 0xffff;
    }
    return NULL;
  }

  int number_of_seq_num = 0;
  if (low_seq_num > high_seq_num) {
    if (low_seq_num - high_seq_num > 0x00ff) {
      
      number_of_seq_num = (0xffff - low_seq_num) + high_seq_num + 1;
    }
  } else {
    number_of_seq_num = high_seq_num - low_seq_num;
  }

  if (number_of_seq_num > kNackHistoryLength) {
    
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "Nack list too large, try to find a key frame and restart "
                 "from seq: %d. Lowest seq in jb %d",
                 high_seq_num, low_seq_num);

    
    bool found_key_frame = false;

    while (number_of_seq_num > kNackHistoryLength) {
      found_key_frame = RecycleFramesUntilKeyFrame();

      if (!found_key_frame) {
        break;
      }

      
      low_seq_num = -1;
      high_seq_num = -1;
      GetLowHighSequenceNumbers(&low_seq_num, &high_seq_num);

      if (high_seq_num == -1) {
        assert(low_seq_num != -1);  
        
        return NULL;
      }

      number_of_seq_num = 0;
      if (low_seq_num > high_seq_num) {
        if (low_seq_num - high_seq_num > 0x00ff) {
          
          number_of_seq_num = (0xffff - low_seq_num) + high_seq_num + 1;
          high_seq_num = low_seq_num;
        }
      } else {
        number_of_seq_num = high_seq_num - low_seq_num;
      }
    }

    if (!found_key_frame) {
      
      
      last_decoded_state_.SetSeqNum(static_cast<uint16_t>(high_seq_num));
      
      *nack_list_size = 0xffff;
      *list_extended = true;
      WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, -1,
                   "\tNo key frame found, request one. last_decoded_seq_num_ "
                   "%d", last_decoded_state_.sequence_num());
    } else {
      
      WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, -1,
                   "\tKey frame found. last_decoded_seq_num_ %d",
                   last_decoded_state_.sequence_num());
      *nack_list_size = 0;
    }

    return NULL;
  }

  uint16_t seq_number_iterator = static_cast<uint16_t>(low_seq_num + 1);
  for (i = 0; i < number_of_seq_num; i++) {
    nack_seq_nums_internal_[i] = seq_number_iterator;
    seq_number_iterator++;
  }
  
  
  for (i = 0; i < max_number_of_frames_; i++) {
    
    
    VCMFrameBufferStateEnum state = frame_buffers_[i]->GetState();
    if (kStateFree != state) {
      
      
      if (nack_mode_ == kNackHybrid) {
        frame_buffers_[i]->BuildSoftNackList(nack_seq_nums_internal_,
                                             number_of_seq_num,
                                             rtt_ms_);
      } else {
        
        
        frame_buffers_[i]->BuildHardNackList(nack_seq_nums_internal_,
                                             number_of_seq_num);
      }
    }
  }

  
  int empty_index = -1;
  for (i = 0; i < number_of_seq_num; i++) {
    if (nack_seq_nums_internal_[i] == -1 || nack_seq_nums_internal_[i] == -2) {
      
      if (empty_index == -1) {
        
        empty_index = i;
      }
    } else {
      
      if (empty_index == -1) {
        
      } else {
        nack_seq_nums_internal_[empty_index] = nack_seq_nums_internal_[i];
        nack_seq_nums_internal_[i] = -1;
        empty_index++;
      }
    }
  }

  if (empty_index == -1) {
    
    *nack_list_size = number_of_seq_num;
  } else {
    *nack_list_size = empty_index;
  }

  if (*nack_list_size > nack_seq_nums_length_) {
    
    *list_extended = true;
  }

  for (unsigned int j = 0; j < *nack_list_size; j++) {
    
    
    if (nack_seq_nums_length_ > j && !*list_extended) {
      unsigned int k = 0;
      for (k = j; k < nack_seq_nums_length_; k++) {
        
        if (nack_seq_nums_[k] ==
            static_cast<uint16_t>(nack_seq_nums_internal_[j])) {
          break;
        }
      }
      if (k == nack_seq_nums_length_) {  
        *list_extended = true;
      }
    } else {
      *list_extended = true;
    }
    nack_seq_nums_[j] = static_cast<uint16_t>(nack_seq_nums_internal_[j]);
  }

  nack_seq_nums_length_ = *nack_list_size;

  return nack_seq_nums_;
}

int64_t VCMJitterBuffer::LastDecodedTimestamp() const {
  CriticalSectionScoped cs(crit_sect_);
  return last_decoded_state_.time_stamp();
}

VCMEncodedFrame* VCMJitterBuffer::GetFrameForDecodingNACK() {
  CleanUpOldFrames();
  
  
  
  if (last_decoded_state_.init()) {
    waiting_for_key_frame_ = true;
  }
  
  bool enable_decodable = nack_mode_ == kNackHybrid ? true : false;
  FrameList::iterator it = FindOldestCompleteContinuousFrame(enable_decodable);
  if (it == frame_list_.end()) {
    
    it = find_if(frame_list_.begin(), frame_list_.end(),
                 CompleteDecodableKeyFrameCriteria());
    if (it == frame_list_.end()) {
      return NULL;
    }
  }
  VCMFrameBuffer* oldest_frame = *it;
  
  const bool retransmitted = (oldest_frame->GetNackCount() > 0);
  if (retransmitted) {
    jitter_estimate_.FrameNacked();
  } else if (oldest_frame->Length() > 0) {
    
    UpdateJitterEstimate(*oldest_frame, false);
  }
  it = frame_list_.erase(it);

  
  VerifyAndSetPreviousFrameLost(oldest_frame);

  
  
  
  oldest_frame->SetState(kStateDecoding);

  
  CleanUpOldFrames();

  if (oldest_frame->FrameType() == kVideoFrameKey) {
    waiting_for_key_frame_ = false;
  }

  
  last_decoded_state_.SetState(oldest_frame);

  return oldest_frame;
}



void VCMJitterBuffer::ReleaseFrameIfNotDecoding(VCMFrameBuffer* frame) {
  if (frame != NULL && frame->GetState() != kStateDecoding) {
    frame->SetState(kStateFree);
  }
}

VCMFrameBuffer* VCMJitterBuffer::GetEmptyFrame() {
  if (!running_) {
    return NULL;
  }

  crit_sect_->Enter();

  for (int i = 0; i < max_number_of_frames_; ++i) {
    if (kStateFree == frame_buffers_[i]->GetState()) {
      
      frame_buffers_[i]->SetState(kStateEmpty);
      crit_sect_->Leave();
      return frame_buffers_[i];
    }
  }

  
  if (max_number_of_frames_ < kMaxNumberOfFrames) {
    VCMFrameBuffer* ptr_new_buffer = new VCMFrameBuffer();
    ptr_new_buffer->SetState(kStateEmpty);
    frame_buffers_[max_number_of_frames_] = ptr_new_buffer;
    max_number_of_frames_++;

    crit_sect_->Leave();
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "JB(0x%x) FB(0x%x): Jitter buffer  increased to:%d frames",
                 this, ptr_new_buffer, max_number_of_frames_);
    return ptr_new_buffer;
  }
  crit_sect_->Leave();

  
  return NULL;
}



bool VCMJitterBuffer::RecycleFramesUntilKeyFrame() {
  
  while (frame_list_.size() > 0) {
    
    drop_count_++;
    FrameList::iterator it = frame_list_.begin();
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "Jitter buffer drop count:%d, low_seq %d", drop_count_,
                 (*it)->GetLowSeqNum());
    ReleaseFrameIfNotDecoding(*it);
    it = frame_list_.erase(it);
    if (it != frame_list_.end() && (*it)->FrameType() == kVideoFrameKey) {
      
      last_decoded_state_.SetStateOneBack(*it);
      return true;
    }
  }
  waiting_for_key_frame_ = true;
  last_decoded_state_.Reset();  
  return false;
}


VCMFrameBufferEnum VCMJitterBuffer::UpdateFrameState(VCMFrameBuffer* frame) {
  if (frame == NULL) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_), "JB(0x%x) FB(0x%x): "
                 "UpdateFrameState NULL frame pointer", this, frame);
    return kNoError;
  }

  int length = frame->Length();
  if (master_) {
    
    
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "JB(0x%x) FB(0x%x): Complete frame added to jitter buffer,"
                 " size:%d type %d",
                 this, frame, length, frame->FrameType());
  }

  if (length != 0 && !frame->GetCountedFrame()) {
    
    incoming_frame_count_++;
    frame->SetCountedFrame(true);
  }

  
  if (last_decoded_state_.IsOldFrame(frame)) {
    
    
    frame->Reset();
    frame->SetState(kStateEmpty);
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "JB(0x%x) FB(0x%x): Dropping old frame in Jitter buffer",
                 this, frame);
    drop_count_++;
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "Jitter buffer drop count: %d, consecutive drops: %u",
                 drop_count_, num_consecutive_old_frames_);
    
    num_consecutive_old_frames_++;
    if (num_consecutive_old_frames_ > kMaxConsecutiveOldFrames) {
      Flush();
      return kFlushIndicator;
    }
    return kNoError;
  }
  num_consecutive_old_frames_ = 0;
  frame->SetState(kStateComplete);

  
  
  if (frame->IsSessionComplete()) {
    switch (frame->FrameType()) {
      case kVideoFrameKey: {
        receive_statistics_[0]++;
        break;
      }
      case kVideoFrameDelta: {
        receive_statistics_[1]++;
        break;
      }
      case kVideoFrameGolden: {
        receive_statistics_[2]++;
        break;
      }
      case kVideoFrameAltRef: {
        receive_statistics_[3]++;
        break;
      }
      default:
        assert(false);
    }
  }
  const FrameList::iterator it = FindOldestCompleteContinuousFrame(false);
  VCMFrameBuffer* old_frame = NULL;
  if (it != frame_list_.end()) {
    old_frame = *it;
  }

  
  
  if (!WaitForRetransmissions() || (old_frame != NULL && old_frame == frame)) {
    frame_event_.Set();
  }
  return kNoError;
}



FrameList::iterator VCMJitterBuffer::FindOldestCompleteContinuousFrame(
    bool enable_decodable) {
  
  VCMFrameBuffer* oldest_frame = NULL;
  FrameList::iterator it = frame_list_.begin();

  
  
  
  
  for (; it != frame_list_.end(); ++it)  {
    oldest_frame = *it;
    VCMFrameBufferStateEnum state = oldest_frame->GetState();
    
    if ((state == kStateComplete ||
         (enable_decodable && state == kStateDecodable)) &&
        last_decoded_state_.ContinuousFrame(oldest_frame)) {
      break;
    } else {
      int temporal_id = oldest_frame->TemporalId();
      oldest_frame = NULL;
      if (temporal_id <= 0) {
        
        
        break;
      }
    }
  }

  if (oldest_frame == NULL) {
    
    return frame_list_.end();
  } else if (waiting_for_key_frame_ &&
              oldest_frame->FrameType() != kVideoFrameKey) {
    
    return frame_list_.end();
  }
  
  return it;
}


void VCMJitterBuffer::CleanUpOldFrames() {
  while (frame_list_.size() > 0) {
    VCMFrameBuffer* oldest_frame = frame_list_.front();
    if (oldest_frame->GetState() == kStateEmpty && frame_list_.size() > 1) {
      
      last_decoded_state_.UpdateEmptyFrame(oldest_frame);
    }
    if (last_decoded_state_.IsOldFrame(oldest_frame)) {
      ReleaseFrameIfNotDecoding(frame_list_.front());
      frame_list_.erase(frame_list_.begin());
    } else {
      break;
    }
  }
}

void VCMJitterBuffer::VerifyAndSetPreviousFrameLost(VCMFrameBuffer* frame) {
  assert(frame);
  frame->MakeSessionDecodable();  
  if (frame->FrameType() == kVideoFrameKey)
    return;

  if (!last_decoded_state_.ContinuousFrame(frame))
    frame->SetPreviousFrameLoss();
}


bool VCMJitterBuffer::IsPacketRetransmitted(const VCMPacket& packet) const {
  if (nack_seq_nums_length_ > 0) {
    for (unsigned int i = 0; i < nack_seq_nums_length_; i++) {
      if (packet.seqNum == nack_seq_nums_[i]) {
        return true;
      }
    }
  }
  return false;
}




void VCMJitterBuffer::UpdateJitterEstimate(const VCMJitterSample& sample,
                                           bool incomplete_frame) {
  if (sample.latest_packet_time == -1) {
    return;
  }
  if (incomplete_frame) {
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_), "Received incomplete frame "
                 "timestamp %u frame size %u at time %u",
                 sample.timestamp, sample.frame_size,
                 MaskWord64ToUWord32(sample.latest_packet_time));
  } else {
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_), "Received complete frame "
                 "timestamp %u frame size %u at time %u",
                 sample.timestamp, sample.frame_size,
                 MaskWord64ToUWord32(sample.latest_packet_time));
  }
  UpdateJitterEstimate(sample.latest_packet_time, sample.timestamp,
                       sample.frame_size, incomplete_frame);
}




void VCMJitterBuffer::UpdateJitterEstimate(const VCMFrameBuffer& frame,
                                           bool incomplete_frame) {
  if (frame.LatestPacketTimeMs() == -1) {
    return;
  }
  
  
  if (incomplete_frame) {
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_),
                 "Received incomplete frame timestamp %u frame type %d "
                 "frame size %u at time %u, jitter estimate was %u",
                 frame.TimeStamp(), frame.FrameType(), frame.Length(),
                 MaskWord64ToUWord32(frame.LatestPacketTimeMs()),
                 EstimatedJitterMs());
  } else {
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(vcm_id_, receiver_id_), "Received complete frame "
                 "timestamp %u frame type %d frame size %u at time %u, "
                 "jitter estimate was %u",
                 frame.TimeStamp(), frame.FrameType(), frame.Length(),
                 MaskWord64ToUWord32(frame.LatestPacketTimeMs()),
                 EstimatedJitterMs());
  }
  UpdateJitterEstimate(frame.LatestPacketTimeMs(), frame.TimeStamp(),
                       frame.Length(), incomplete_frame);
}




void VCMJitterBuffer::UpdateJitterEstimate(
    int64_t latest_packet_time_ms,
    uint32_t timestamp,
    unsigned int frame_size,
    bool incomplete_frame) {
  if (latest_packet_time_ms == -1) {
    return;
  }
  int64_t frame_delay;
  
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
               VCMId(vcm_id_, receiver_id_),
               "Packet received and sent to jitter estimate with: "
               "timestamp=%u wall_clock=%u", timestamp,
               MaskWord64ToUWord32(latest_packet_time_ms));
  bool not_reordered = inter_frame_delay_.CalculateDelay(timestamp,
                                                      &frame_delay,
                                                      latest_packet_time_ms);
  
  if (not_reordered) {
    
    jitter_estimate_.UpdateEstimate(frame_delay, frame_size, incomplete_frame);
  }
}


void VCMJitterBuffer::GetLowHighSequenceNumbers(
    int32_t* low_seq_num, int32_t* high_seq_num) const {
  assert(low_seq_num);
  assert(high_seq_num);
  
  int i = 0;
  int32_t seq_num = -1;

  *high_seq_num = -1;
  *low_seq_num = -1;
  if (!last_decoded_state_.init())
    *low_seq_num = last_decoded_state_.sequence_num();

  
  for (i = 0; i < max_number_of_frames_; ++i) {
    seq_num = frame_buffers_[i]->GetHighSeqNum();

    
    VCMFrameBufferStateEnum state = frame_buffers_[i]->GetState();

    if ((kStateFree != state) &&
        (kStateEmpty != state) &&
        (kStateDecoding != state) &&
        seq_num != -1) {
      bool wrap;
      *high_seq_num = LatestSequenceNumber(seq_num, *high_seq_num, &wrap);
    }
  }
}

bool VCMJitterBuffer::WaitForRetransmissions() {
  if (nack_mode_ == kNoNack) {
    
    return false;
  } else if (nack_mode_ == kNackInfinite) {
    
    return true;
  }
  
  
  if (high_rtt_nack_threshold_ms_ >= 0 &&
      rtt_ms_ >= static_cast<unsigned int>(high_rtt_nack_threshold_ms_)) {
    return false;
  }
  return true;
}
}  
