








#include "webrtc/modules/video_coding/main/source/jitter_buffer.h"

#include <assert.h>

#include <algorithm>
#include <utility>

#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/source/frame_buffer.h"
#include "webrtc/modules/video_coding/main/source/inter_frame_delay.h"
#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/jitter_buffer_common.h"
#include "webrtc/modules/video_coding/main/source/jitter_estimator.h"
#include "webrtc/modules/video_coding/main/source/packet.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/metrics.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace webrtc {


static const uint32_t kDefaultRtt = 200;

typedef std::pair<uint32_t, VCMFrameBuffer*> FrameListPair;

bool IsKeyFrame(FrameListPair pair) {
  return pair.second->FrameType() == kVideoFrameKey;
}

bool HasNonEmptyState(FrameListPair pair) {
  return pair.second->GetState() != kStateEmpty;
}

void FrameList::InsertFrame(VCMFrameBuffer* frame) {
  insert(rbegin().base(), FrameListPair(frame->TimeStamp(), frame));
}




























VCMFrameBuffer* FrameList::FindFrame(uint16_t seq_num, uint32_t timestamp) const {
  FrameList::const_iterator it = find(timestamp);
  
  
  

  
  
  
  while (it != end() && it->second->GetState() == kStateComplete) {
    it++;
  }
  if (it == end())
    return NULL;
  return it->second;
}

VCMFrameBuffer* FrameList::PopFrame(uint32_t timestamp) {
  FrameList::iterator it = find(timestamp);
  if (it == end())
    return NULL;
  VCMFrameBuffer* frame = it->second;
  erase(it);
  return frame;
}

VCMFrameBuffer* FrameList::Front() const {
  return begin()->second;
}

VCMFrameBuffer* FrameList::Back() const {
  return rbegin()->second;
}

int FrameList::RecycleFramesUntilKeyFrame(FrameList::iterator* key_frame_it,
                                          UnorderedFrameList* free_frames) {
  int drop_count = 0;
  FrameList::iterator it = begin();
  while (!empty()) {
    
    it->second->Reset();
    free_frames->push_back(it->second);
    erase(it++);
    ++drop_count;
    if (it != end() && it->second->FrameType() == kVideoFrameKey) {
      *key_frame_it = it;
      return drop_count;
    }
  }
  *key_frame_it = end();
  return drop_count;
}

int FrameList::CleanUpOldOrEmptyFrames(VCMDecodingState* decoding_state,
                                       UnorderedFrameList* free_frames) {
  int drop_count = 0;
  while (!empty()) {
    VCMFrameBuffer* oldest_frame = Front();
    bool remove_frame = false;
    if (oldest_frame->GetState() == kStateEmpty && size() > 1) {
      
      
      remove_frame = decoding_state->UpdateEmptyFrame(oldest_frame);
    } else {
      remove_frame = decoding_state->IsOldFrame(oldest_frame);
    }
    if (!remove_frame) {
      break;
    }
    free_frames->push_back(oldest_frame);
    ++drop_count;
    TRACE_EVENT_INSTANT1("webrtc", "JB::OldOrEmptyFrameDropped", "timestamp",
                         oldest_frame->TimeStamp());
    erase(begin());
  }
  return drop_count;
}

void FrameList::Reset(UnorderedFrameList* free_frames) {
  while (!empty()) {
    begin()->second->Reset();
    free_frames->push_back(begin()->second);
    erase(begin());
  }
}

VCMJitterBuffer::VCMJitterBuffer(Clock* clock, EventFactory* event_factory)
    : clock_(clock),
      running_(false),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      frame_event_(event_factory->CreateEvent()),
      packet_event_(event_factory->CreateEvent()),
      max_number_of_frames_(kStartNumberOfFrames),
      frame_buffers_(),
      free_frames_(),
      decodable_frames_(),
      incomplete_frames_(),
      last_decoded_state_(),
      first_packet_since_reset_(true),
      incoming_frame_rate_(0),
      incoming_frame_count_(0),
      time_last_incoming_frame_count_(0),
      incoming_bit_count_(0),
      incoming_bit_rate_(0),
      drop_count_(0),
      num_consecutive_old_frames_(0),
      num_consecutive_old_packets_(0),
      num_packets_(0),
      num_duplicated_packets_(0),
      num_discarded_packets_(0),
      jitter_estimate_(clock),
      inter_frame_delay_(clock_->TimeInMilliseconds()),
      rtt_ms_(kDefaultRtt),
      nack_mode_(kNoNack),
      low_rtt_nack_threshold_ms_(-1),
      high_rtt_nack_threshold_ms_(-1),
      missing_sequence_numbers_(SequenceNumberLessThan()),
      nack_seq_nums_(),
      max_nack_list_size_(0),
      max_packet_age_to_nack_(0),
      max_incomplete_time_ms_(0),
      decode_error_mode_(kNoErrors),
      average_packets_per_frame_(0.0f),
      frame_counter_(0) {
  memset(frame_buffers_, 0, sizeof(frame_buffers_));

  for (int i = 0; i < kStartNumberOfFrames; i++) {
    frame_buffers_[i] = new VCMFrameBuffer();
    free_frames_.push_back(frame_buffers_[i]);
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
    running_ = rhs.running_;
    max_number_of_frames_ = rhs.max_number_of_frames_;
    incoming_frame_rate_ = rhs.incoming_frame_rate_;
    incoming_frame_count_ = rhs.incoming_frame_count_;
    time_last_incoming_frame_count_ = rhs.time_last_incoming_frame_count_;
    incoming_bit_count_ = rhs.incoming_bit_count_;
    incoming_bit_rate_ = rhs.incoming_bit_rate_;
    drop_count_ = rhs.drop_count_;
    num_consecutive_old_frames_ = rhs.num_consecutive_old_frames_;
    num_consecutive_old_packets_ = rhs.num_consecutive_old_packets_;
    num_packets_ = rhs.num_packets_;
    num_duplicated_packets_ = rhs.num_duplicated_packets_;
    num_discarded_packets_ = rhs.num_discarded_packets_;
    jitter_estimate_ = rhs.jitter_estimate_;
    inter_frame_delay_ = rhs.inter_frame_delay_;
    waiting_for_completion_ = rhs.waiting_for_completion_;
    rtt_ms_ = rhs.rtt_ms_;
    first_packet_since_reset_ = rhs.first_packet_since_reset_;
    last_decoded_state_ =  rhs.last_decoded_state_;
    decode_error_mode_ = rhs.decode_error_mode_;
    assert(max_nack_list_size_ == rhs.max_nack_list_size_);
    assert(max_packet_age_to_nack_ == rhs.max_packet_age_to_nack_);
    assert(max_incomplete_time_ms_ == rhs.max_incomplete_time_ms_);
    receive_statistics_ = rhs.receive_statistics_;
    nack_seq_nums_.resize(rhs.nack_seq_nums_.size());
    missing_sequence_numbers_ = rhs.missing_sequence_numbers_;
    latest_received_sequence_number_ = rhs.latest_received_sequence_number_;
    average_packets_per_frame_ = rhs.average_packets_per_frame_;
    for (int i = 0; i < kMaxNumberOfFrames; i++) {
      if (frame_buffers_[i] != NULL) {
        delete frame_buffers_[i];
        frame_buffers_[i] = NULL;
      }
    }
    free_frames_.clear();
    decodable_frames_.clear();
    incomplete_frames_.clear();
    int i = 0;
    for (UnorderedFrameList::const_iterator it = rhs.free_frames_.begin();
         it != rhs.free_frames_.end(); ++it, ++i) {
      frame_buffers_[i] = new VCMFrameBuffer;
      free_frames_.push_back(frame_buffers_[i]);
    }
    CopyFrames(&decodable_frames_, rhs.decodable_frames_, &i);
    CopyFrames(&incomplete_frames_, rhs.incomplete_frames_, &i);
    rhs.crit_sect_->Leave();
    crit_sect_->Leave();
  }
}

void VCMJitterBuffer::CopyFrames(FrameList* to_list,
    const FrameList& from_list, int* index) {
  to_list->clear();
  for (FrameList::const_iterator it = from_list.begin();
       it != from_list.end(); ++it, ++*index) {
    frame_buffers_[*index] = new VCMFrameBuffer(*it->second);
    to_list->InsertFrame(frame_buffers_[*index]);
  }
}

void VCMJitterBuffer::UpdateHistograms() {
  if (num_packets_ > 0) {
    RTC_HISTOGRAM_PERCENTAGE("WebRTC.Video.DiscardedPacketsInPercent",
        num_discarded_packets_ * 100 / num_packets_);
    RTC_HISTOGRAM_PERCENTAGE("WebRTC.Video.DuplicatedPacketsInPercent",
        num_duplicated_packets_ * 100 / num_packets_);
  }
}

void VCMJitterBuffer::Start() {
  CriticalSectionScoped cs(crit_sect_);
  running_ = true;
  incoming_frame_count_ = 0;
  incoming_frame_rate_ = 0;
  incoming_bit_count_ = 0;
  incoming_bit_rate_ = 0;
  time_last_incoming_frame_count_ = clock_->TimeInMilliseconds();
  receive_statistics_.clear();

  num_consecutive_old_frames_ = 0;
  num_consecutive_old_packets_ = 0;
  num_packets_ = 0;
  num_duplicated_packets_ = 0;
  num_discarded_packets_ = 0;

  
  frame_event_->Reset();
  packet_event_->Reset();
  waiting_for_completion_.frame_size = 0;
  waiting_for_completion_.timestamp = 0;
  waiting_for_completion_.latest_packet_time = -1;
  first_packet_since_reset_ = true;
  rtt_ms_ = kDefaultRtt;
  last_decoded_state_.Reset();
}

void VCMJitterBuffer::Stop() {
  crit_sect_->Enter();
  UpdateHistograms();
  running_ = false;
  last_decoded_state_.Reset();
  free_frames_.clear();
  decodable_frames_.clear();
  incomplete_frames_.clear();
  
  for (int i = 0; i < kMaxNumberOfFrames; i++) {
    if (frame_buffers_[i] != NULL) {
      static_cast<VCMFrameBuffer*>(frame_buffers_[i])->Reset();
      free_frames_.push_back(frame_buffers_[i]);
    }
  }
  crit_sect_->Leave();
  
  frame_event_->Set();
  packet_event_->Set();
}

bool VCMJitterBuffer::Running() const {
  CriticalSectionScoped cs(crit_sect_);
  return running_;
}

void VCMJitterBuffer::Flush() {
  CriticalSectionScoped cs(crit_sect_);
  decodable_frames_.Reset(&free_frames_);
  incomplete_frames_.Reset(&free_frames_);
  last_decoded_state_.Reset();  
  frame_event_->Reset();
  packet_event_->Reset();
  num_consecutive_old_frames_ = 0;
  num_consecutive_old_packets_ = 0;
  
  jitter_estimate_.Reset();
  inter_frame_delay_.Reset(clock_->TimeInMilliseconds());
  waiting_for_completion_.frame_size = 0;
  waiting_for_completion_.timestamp = 0;
  waiting_for_completion_.latest_packet_time = -1;
  first_packet_since_reset_ = true;
  missing_sequence_numbers_.clear();
}


std::map<FrameType, uint32_t> VCMJitterBuffer::FrameStatistics() const {
  CriticalSectionScoped cs(crit_sect_);
  return receive_statistics_;
}

int VCMJitterBuffer::num_packets() const {
  CriticalSectionScoped cs(crit_sect_);
  return num_packets_;
}

int VCMJitterBuffer::num_duplicated_packets() const {
  CriticalSectionScoped cs(crit_sect_);
  return num_duplicated_packets_;
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
  const int64_t now = clock_->TimeInMilliseconds();
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
    
    time_last_incoming_frame_count_ = clock_->TimeInMilliseconds();
    *framerate = 0;
    *bitrate = 0;
    incoming_frame_rate_ = 0;
    incoming_bit_rate_ = 0;
  }
}






bool VCMJitterBuffer::CompleteSequenceWithNextFrame() {
  CriticalSectionScoped cs(crit_sect_);
  
  CleanUpOldOrEmptyFrames();
  if (!decodable_frames_.empty()) {
    if (decodable_frames_.Front()->GetState() == kStateComplete) {
      return true;
    }
  } else if (incomplete_frames_.size() <= 1) {
    
    return true;
  }
  return false;
}



bool VCMJitterBuffer::NextCompleteTimestamp(
    uint32_t max_wait_time_ms, uint32_t* timestamp) {
  crit_sect_->Enter();
  if (!running_) {
    crit_sect_->Leave();
    return false;
  }
  CleanUpOldOrEmptyFrames();

  if (decodable_frames_.empty() ||
      decodable_frames_.Front()->GetState() != kStateComplete) {
    const int64_t end_wait_time_ms = clock_->TimeInMilliseconds() +
        max_wait_time_ms;
    int64_t wait_time_ms = max_wait_time_ms;
    while (wait_time_ms > 0) {
      crit_sect_->Leave();
      const EventTypeWrapper ret =
        frame_event_->Wait(static_cast<uint32_t>(wait_time_ms));
      crit_sect_->Enter();
      if (ret == kEventSignaled) {
        
        if (!running_) {
          crit_sect_->Leave();
          return false;
        }
        
        CleanUpOldOrEmptyFrames();
        if (decodable_frames_.empty() ||
            decodable_frames_.Front()->GetState() != kStateComplete) {
          wait_time_ms = end_wait_time_ms - clock_->TimeInMilliseconds();
        } else {
          break;
        }
      } else {
        break;
      }
    }
    
  } else {
    
    frame_event_->Reset();
  }
  if (decodable_frames_.empty() ||
      decodable_frames_.Front()->GetState() != kStateComplete) {
    crit_sect_->Leave();
    return false;
  }
  *timestamp = decodable_frames_.Front()->TimeStamp();
  crit_sect_->Leave();
  return true;
}

bool VCMJitterBuffer::NextMaybeIncompleteTimestamp(uint32_t* timestamp) {
  CriticalSectionScoped cs(crit_sect_);
  if (!running_) {
    return false;
  }
  if (decode_error_mode_ == kNoErrors) {
    
    return false;
  }

  CleanUpOldOrEmptyFrames();

  if (decodable_frames_.empty()) {
    return false;
  }
  VCMFrameBuffer* oldest_frame = decodable_frames_.Front();
  
  
  
  if (decodable_frames_.size() == 1 && incomplete_frames_.empty()
      && oldest_frame->GetState() != kStateComplete) {
    return false;
  }

  *timestamp = oldest_frame->TimeStamp();
  return true;
}

VCMEncodedFrame* VCMJitterBuffer::ExtractAndSetDecode(uint32_t timestamp) {
  CriticalSectionScoped cs(crit_sect_);
  if (!running_) {
    return NULL;
  }
  
  VCMFrameBuffer* frame = decodable_frames_.PopFrame(timestamp);
  bool continuous = true;
  if (!frame) {
    frame = incomplete_frames_.PopFrame(timestamp);
    if (frame)
      continuous = last_decoded_state_.ContinuousFrame(frame);
    else
      return NULL;
  }
  TRACE_EVENT_ASYNC_STEP0("webrtc", "Video", timestamp, "Extract");
  
  const bool retransmitted = (frame->GetNackCount() > 0);
  if (retransmitted) {
    jitter_estimate_.FrameNacked();
  } else if (frame->Length() > 0) {
    
    if (waiting_for_completion_.latest_packet_time >= 0) {
      UpdateJitterEstimate(waiting_for_completion_, true);
    }
    if (frame->GetState() == kStateComplete) {
      UpdateJitterEstimate(*frame, false);
    } else {
      
      waiting_for_completion_.frame_size = frame->Length();
      waiting_for_completion_.latest_packet_time =
          frame->LatestPacketTimeMs();
      waiting_for_completion_.timestamp = frame->TimeStamp();
    }
  }

  
  
  
  frame->PrepareForDecode(continuous);

  
  last_decoded_state_.SetState(frame);
  DropPacketsFromNackList(last_decoded_state_.sequence_num());

  if ((*frame).IsSessionComplete())
    UpdateAveragePacketsPerFrame(frame->NumPackets());

  if (frame->Length() == 0) {
    
    ReleaseFrame(frame);
    return NULL;
  }
  return frame;
}



void VCMJitterBuffer::ReleaseFrame(VCMEncodedFrame* frame) {
  CriticalSectionScoped cs(crit_sect_);
  VCMFrameBuffer* frame_buffer = static_cast<VCMFrameBuffer*>(frame);
  if (frame_buffer) {
    free_frames_.push_back(frame_buffer);
  }
}


VCMFrameBufferEnum VCMJitterBuffer::GetFrame(const VCMPacket& packet,
                                             VCMFrameBuffer** frame) {
  ++num_packets_;
  
  if (last_decoded_state_.IsOldPacket(&packet)) {
    
    if (packet.sizeBytes > 0) {
      num_discarded_packets_++;
      num_consecutive_old_packets_++;
    }
    
    
    
    last_decoded_state_.UpdateOldPacket(&packet);
    DropPacketsFromNackList(last_decoded_state_.sequence_num());

    if (num_consecutive_old_packets_ > kMaxConsecutiveOldPackets) {
      LOG(LS_WARNING) << num_consecutive_old_packets_ << " consecutive old "
                         "packets received. Flushing the jitter buffer.";
      Flush();
      return kFlushIndicator;
    }
    return kOldPacket;
  }
  num_consecutive_old_packets_ = 0;

  
  

  
  
  
  
  if (packet.completeNALU != kNaluComplete) {
    *frame = incomplete_frames_.FindFrame(packet.seqNum, packet.timestamp);
    if (*frame)
      return kNoError;
    *frame = decodable_frames_.FindFrame(packet.seqNum, packet.timestamp);
    if (*frame && (*frame)->GetState() != kStateComplete)
      return kNoError;
  }

  
  *frame = GetEmptyFrame();
  VCMFrameBufferEnum ret = kNoError;
  if (!*frame) {
    
    LOG(LS_WARNING) << "Unable to get empty frame; Recycling.";
    bool found_key_frame = RecycleFramesUntilKeyFrame();
    *frame = GetEmptyFrame();
    if (!*frame) {
      LOG(LS_ERROR) << "GetEmptyFrame returned NULL.";
      return kGeneralError;
    } else if (!found_key_frame) {
      ret = kFlushIndicator;
    }
  }
  (*frame)->Reset();
  return ret;
}

int64_t VCMJitterBuffer::LastPacketTime(const VCMEncodedFrame* frame,
                                        bool* retransmitted) const {
  assert(retransmitted);
  CriticalSectionScoped cs(crit_sect_);
  const VCMFrameBuffer* frame_buffer =
      static_cast<const VCMFrameBuffer*>(frame);
  *retransmitted = (frame_buffer->GetNackCount() > 0);
  return frame_buffer->LatestPacketTimeMs();
}

VCMFrameBufferEnum VCMJitterBuffer::InsertPacket(const VCMPacket& packet,
                                                 bool* retransmitted) {
  CriticalSectionScoped cs(crit_sect_);

  VCMFrameBuffer* frame = NULL;
  const VCMFrameBufferEnum error = GetFrame(packet, &frame);
  if (error != kNoError && frame == NULL) {
    return error;
  }
  int64_t now_ms = clock_->TimeInMilliseconds();
  
  
  if (first_packet_since_reset_) {
    
    
    inter_frame_delay_.Reset(now_ms);
  }
  if (last_decoded_state_.IsOldPacket(&packet)) {
    
    
    last_decoded_state_.UpdateOldPacket(&packet);
    drop_count_++;
    
    num_consecutive_old_frames_++;
    if (num_consecutive_old_frames_ > kMaxConsecutiveOldFrames) {
      LOG(LS_WARNING) << num_consecutive_old_packets_ << " consecutive old "
                         "frames received. Flushing the jitter buffer.";
      Flush();
      return kFlushIndicator;
    }
    return kNoError;
  }

  num_consecutive_old_frames_ = 0;

  
  
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

  VCMFrameBufferStateEnum previous_state = frame->GetState();
  
  
  
  bool first = (frame->GetHighSeqNum() == -1);
  FrameData frame_data;
  frame_data.rtt_ms = rtt_ms_;
  frame_data.rolling_average_packets_per_frame = average_packets_per_frame_;
  VCMFrameBufferEnum buffer_return = frame->InsertPacket(packet,
                                                         now_ms,
                                                         decode_error_mode_,
                                                         frame_data);
  if (!frame->GetCountedFrame()) {
    TRACE_EVENT_ASYNC_BEGIN1("webrtc", "Video", frame->TimeStamp(),
                             "timestamp", frame->TimeStamp());
  }

  if (buffer_return > 0) {
    incoming_bit_count_ += packet.sizeBytes << 3;
    if (first_packet_since_reset_) {
      latest_received_sequence_number_ = packet.seqNum;
      first_packet_since_reset_ = false;
    } else {
      if (IsPacketRetransmitted(packet)) {
        frame->IncrementNackCount();
      }
      if (!UpdateNackList(packet.seqNum) &&
          packet.frameType != kVideoFrameKey) {
        buffer_return = kFlushIndicator;
      }
      latest_received_sequence_number_ = LatestSequenceNumber(
          latest_received_sequence_number_, packet.seqNum);
    }
  }

  
  bool update_decodable_list = (previous_state != kStateDecodable &&
      previous_state != kStateComplete);
  bool continuous = IsContinuous(*frame);
  switch (buffer_return) {
    case kGeneralError:
    case kTimeStampError:
    case kSizeError: {
      
      frame->Reset();
      break;
    }
    case kCompleteSession: {
      if (update_decodable_list) {
        CountFrame(*frame);
        frame->SetCountedFrame(true);
        if (continuous) {
          
          frame_event_->Set();
        }
      }
    }
    
    case kDecodableSession: {
      *retransmitted = (frame->GetNackCount() > 0);
      
      packet_event_->Set();
      if (!update_decodable_list) {
        break;
      }
      if (continuous) {
        if (!first) {
          incomplete_frames_.PopFrame(packet.timestamp);
        }
        decodable_frames_.InsertFrame(frame);
        FindAndInsertContinuousFrames(*frame);
      } else if (first) {
        incomplete_frames_.InsertFrame(frame);
      }
      break;
    }
    case kIncomplete: {
      
      if (frame->GetState() == kStateEmpty &&
          last_decoded_state_.UpdateEmptyFrame(frame)) {
        free_frames_.push_back(frame);
        frame->Reset();
        frame = NULL;
        return kNoError;
      } else if (first) {
        incomplete_frames_.InsertFrame(frame);
      }
      
      packet_event_->Set();
      break;
    }
    case kNoError:
    case kOutOfBoundsPacket:
    case kDuplicatePacket: {
      ++num_duplicated_packets_;
      break;
    }
    case kFlushIndicator:
      return kFlushIndicator;
    default: {
      assert(false && "JitterBuffer::InsertPacket: Undefined value");
    }
  }
  return buffer_return;
}

bool VCMJitterBuffer::IsContinuousInState(const VCMFrameBuffer& frame,
    const VCMDecodingState& decoding_state) const {
  if (decode_error_mode_ == kWithErrors)
    return true;
  
  
  
  
  if ((frame.GetState() == kStateComplete ||
       frame.GetState() == kStateDecodable) &&
       decoding_state.ContinuousFrame(&frame)) {
    return true;
  } else {
    return false;
  }
}

bool VCMJitterBuffer::IsContinuous(const VCMFrameBuffer& frame) const {
  if (IsContinuousInState(frame, last_decoded_state_)) {
    return true;
  }
  VCMDecodingState decoding_state;
  decoding_state.CopyFrom(last_decoded_state_);
  for (FrameList::const_iterator it = decodable_frames_.begin();
       it != decodable_frames_.end(); ++it)  {
    VCMFrameBuffer* decodable_frame = it->second;
    if (IsNewerTimestamp(decodable_frame->TimeStamp(), frame.TimeStamp())) {
      break;
    }
    decoding_state.SetState(decodable_frame);
    if (IsContinuousInState(frame, decoding_state)) {
      return true;
    }
  }
  return false;
}

void VCMJitterBuffer::FindAndInsertContinuousFrames(
    const VCMFrameBuffer& new_frame) {
  VCMDecodingState decoding_state;
  decoding_state.CopyFrom(last_decoded_state_);
  decoding_state.SetState(&new_frame);
  
  
  
  
  for (FrameList::iterator it = incomplete_frames_.begin();
       it != incomplete_frames_.end();)  {
    VCMFrameBuffer* frame = it->second;
    if (IsNewerTimestamp(new_frame.TimeStamp(), frame->TimeStamp())) {
      ++it;
      continue;
    }
    if (IsContinuousInState(*frame, decoding_state)) {
      decodable_frames_.InsertFrame(frame);
      incomplete_frames_.erase(it++);
      decoding_state.SetState(frame);
    } else if (frame->TemporalId() <= 0) {
      break;
    } else {
      ++it;
    }
  }
}

uint32_t VCMJitterBuffer::EstimatedJitterMs() {
  CriticalSectionScoped cs(crit_sect_);
  
  
  double rtt_mult = 1.0f;
  if (low_rtt_nack_threshold_ms_ >= 0 &&
      static_cast<int>(rtt_ms_) >= low_rtt_nack_threshold_ms_) {
    
    
    rtt_mult = 0.0f;
  }
  return jitter_estimate_.GetJitterEstimate(rtt_mult);
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
  if (mode == kNoNack) {
    missing_sequence_numbers_.clear();
  }
  assert(low_rtt_nack_threshold_ms >= -1 && high_rtt_nack_threshold_ms >= -1);
  assert(high_rtt_nack_threshold_ms == -1 ||
         low_rtt_nack_threshold_ms <= high_rtt_nack_threshold_ms);
  assert(low_rtt_nack_threshold_ms > -1 || high_rtt_nack_threshold_ms == -1);
  low_rtt_nack_threshold_ms_ = low_rtt_nack_threshold_ms;
  high_rtt_nack_threshold_ms_ = high_rtt_nack_threshold_ms;
  
  
  if (rtt_ms_ == kDefaultRtt && high_rtt_nack_threshold_ms_ != -1) {
    rtt_ms_ = 0;
  }
  if (!WaitForRetransmissions()) {
    jitter_estimate_.ResetNackCount();
  }
}

void VCMJitterBuffer::SetNackSettings(size_t max_nack_list_size,
                                      int max_packet_age_to_nack,
                                      int max_incomplete_time_ms) {
  CriticalSectionScoped cs(crit_sect_);
  assert(max_packet_age_to_nack >= 0);
  assert(max_incomplete_time_ms_ >= 0);
  max_nack_list_size_ = max_nack_list_size;
  max_packet_age_to_nack_ = max_packet_age_to_nack;
  max_incomplete_time_ms_ = max_incomplete_time_ms;
  nack_seq_nums_.resize(max_nack_list_size_);
}

VCMNackMode VCMJitterBuffer::nack_mode() const {
  CriticalSectionScoped cs(crit_sect_);
  return nack_mode_;
}

int VCMJitterBuffer::NonContinuousOrIncompleteDuration() {
  if (incomplete_frames_.empty()) {
    return 0;
  }
  uint32_t start_timestamp = incomplete_frames_.Front()->TimeStamp();
  if (!decodable_frames_.empty()) {
    start_timestamp = decodable_frames_.Back()->TimeStamp();
  }
  return incomplete_frames_.Back()->TimeStamp() - start_timestamp;
}

uint16_t VCMJitterBuffer::EstimatedLowSequenceNumber(
    const VCMFrameBuffer& frame) const {
  assert(frame.GetLowSeqNum() >= 0);
  if (frame.HaveFirstPacket())
    return frame.GetLowSeqNum();

  
  
  return frame.GetLowSeqNum() - 1;
}

uint16_t* VCMJitterBuffer::GetNackList(uint16_t* nack_list_size,
                                       bool* request_key_frame) {
  CriticalSectionScoped cs(crit_sect_);
  *request_key_frame = false;
  if (nack_mode_ == kNoNack) {
    *nack_list_size = 0;
    return NULL;
  }
  if (last_decoded_state_.in_initial_state()) {
    VCMFrameBuffer* next_frame =  NextFrame();
    const bool first_frame_is_key = next_frame &&
        next_frame->FrameType() == kVideoFrameKey &&
        next_frame->HaveFirstPacket();
    if (!first_frame_is_key) {
      bool have_non_empty_frame = decodable_frames_.end() != find_if(
          decodable_frames_.begin(), decodable_frames_.end(),
          HasNonEmptyState);
      if (!have_non_empty_frame) {
        have_non_empty_frame = incomplete_frames_.end() != find_if(
            incomplete_frames_.begin(), incomplete_frames_.end(),
            HasNonEmptyState);
      }
      bool found_key_frame = RecycleFramesUntilKeyFrame();
      if (!found_key_frame) {
        *request_key_frame = have_non_empty_frame;
        *nack_list_size = 0;
        return NULL;
      }
    }
  }
  if (TooLargeNackList()) {
    *request_key_frame = !HandleTooLargeNackList();
  }
  if (max_incomplete_time_ms_ > 0) {
    int non_continuous_incomplete_duration =
        NonContinuousOrIncompleteDuration();
    if (non_continuous_incomplete_duration > 90 * max_incomplete_time_ms_) {
      LOG_F(LS_WARNING) << "Too long non-decodable duration: "
                        << non_continuous_incomplete_duration << " > "
                        << 90 * max_incomplete_time_ms_;
      FrameList::reverse_iterator rit = find_if(incomplete_frames_.rbegin(),
          incomplete_frames_.rend(), IsKeyFrame);
      if (rit == incomplete_frames_.rend()) {
        
        *request_key_frame = true;
        *nack_list_size = 0;
        return NULL;
      } else {
        
        
        
        
        last_decoded_state_.Reset();
        DropPacketsFromNackList(EstimatedLowSequenceNumber(*rit->second));
      }
    }
  }
  unsigned int i = 0;
  SequenceNumberSet::iterator it = missing_sequence_numbers_.begin();
  for (; it != missing_sequence_numbers_.end(); ++it, ++i) {
    nack_seq_nums_[i] = *it;
  }
  *nack_list_size = i;
  return &nack_seq_nums_[0];
}

void VCMJitterBuffer::SetDecodeErrorMode(VCMDecodeErrorMode error_mode) {
  CriticalSectionScoped cs(crit_sect_);
  decode_error_mode_ = error_mode;
}

VCMFrameBuffer* VCMJitterBuffer::NextFrame() const {
  if (!decodable_frames_.empty())
    return decodable_frames_.Front();
  if (!incomplete_frames_.empty())
    return incomplete_frames_.Front();
  return NULL;
}

bool VCMJitterBuffer::UpdateNackList(uint16_t sequence_number) {
  if (nack_mode_ == kNoNack) {
    return true;
  }
  
  if (!last_decoded_state_.in_initial_state()) {
    latest_received_sequence_number_ = LatestSequenceNumber(
        latest_received_sequence_number_,
        last_decoded_state_.sequence_num());
  }
  if (IsNewerSequenceNumber(sequence_number,
                            latest_received_sequence_number_)) {
    
    for (uint16_t i = latest_received_sequence_number_ + 1;
         IsNewerSequenceNumber(sequence_number, i); ++i) {
      missing_sequence_numbers_.insert(missing_sequence_numbers_.end(), i);
      TRACE_EVENT_INSTANT1("webrtc", "AddNack", "seqnum", i);
    }
    if (TooLargeNackList() && !HandleTooLargeNackList()) {
      LOG(LS_WARNING) << "Requesting key frame due to too large NACK list.";
      return false;
    }
    if (MissingTooOldPacket(sequence_number) &&
        !HandleTooOldPackets(sequence_number)) {
      LOG(LS_WARNING) << "Requesting key frame due to missing too old packets";
      return false;
    }
  } else {
    missing_sequence_numbers_.erase(sequence_number);
    TRACE_EVENT_INSTANT1("webrtc", "RemoveNack", "seqnum", sequence_number);
  }
  return true;
}

bool VCMJitterBuffer::TooLargeNackList() const {
  return missing_sequence_numbers_.size() > max_nack_list_size_;
}

bool VCMJitterBuffer::HandleTooLargeNackList() {
  
  
  LOG_F(LS_WARNING) << "NACK list has grown too large: "
                    << missing_sequence_numbers_.size() << " > "
                    << max_nack_list_size_;
  bool key_frame_found = false;
  while (TooLargeNackList()) {
    key_frame_found = RecycleFramesUntilKeyFrame();
  }
  return key_frame_found;
}

bool VCMJitterBuffer::MissingTooOldPacket(
    uint16_t latest_sequence_number) const {
  if (missing_sequence_numbers_.empty()) {
    return false;
  }
  const uint16_t age_of_oldest_missing_packet = latest_sequence_number -
      *missing_sequence_numbers_.begin();
  
  
  return age_of_oldest_missing_packet > max_packet_age_to_nack_;
}

bool VCMJitterBuffer::HandleTooOldPackets(uint16_t latest_sequence_number) {
  bool key_frame_found = false;
  const uint16_t age_of_oldest_missing_packet = latest_sequence_number -
      *missing_sequence_numbers_.begin();
  LOG_F(LS_WARNING) << "NACK list contains too old sequence numbers: "
                    << age_of_oldest_missing_packet << " > "
                    << max_packet_age_to_nack_;
  while (MissingTooOldPacket(latest_sequence_number)) {
    key_frame_found = RecycleFramesUntilKeyFrame();
  }
  return key_frame_found;
}

void VCMJitterBuffer::DropPacketsFromNackList(
    uint16_t last_decoded_sequence_number) {
  
  
  missing_sequence_numbers_.erase(missing_sequence_numbers_.begin(),
                                  missing_sequence_numbers_.upper_bound(
                                      last_decoded_sequence_number));
}

int64_t VCMJitterBuffer::LastDecodedTimestamp() const {
  CriticalSectionScoped cs(crit_sect_);
  return last_decoded_state_.time_stamp();
}

void VCMJitterBuffer::RenderBufferSize(uint32_t* timestamp_start,
                                       uint32_t* timestamp_end) {
  CriticalSectionScoped cs(crit_sect_);
  CleanUpOldOrEmptyFrames();
  *timestamp_start = 0;
  *timestamp_end = 0;
  if (decodable_frames_.empty()) {
    return;
  }
  *timestamp_start = decodable_frames_.Front()->TimeStamp();
  *timestamp_end = decodable_frames_.Back()->TimeStamp();
}

VCMFrameBuffer* VCMJitterBuffer::GetEmptyFrame() {
  if (free_frames_.empty()) {
    if (!TryToIncreaseJitterBufferSize()) {
      return NULL;
    }
  }
  VCMFrameBuffer* frame = free_frames_.front();
  free_frames_.pop_front();
  return frame;
}

bool VCMJitterBuffer::TryToIncreaseJitterBufferSize() {
  if (max_number_of_frames_ >= kMaxNumberOfFrames)
    return false;
  VCMFrameBuffer* new_frame = new VCMFrameBuffer();
  frame_buffers_[max_number_of_frames_] = new_frame;
  free_frames_.push_back(new_frame);
  ++max_number_of_frames_;
  TRACE_COUNTER1("webrtc", "JBMaxFrames", max_number_of_frames_);
  return true;
}



bool VCMJitterBuffer::RecycleFramesUntilKeyFrame() {
  
  
  FrameList::iterator key_frame_it;
  bool key_frame_found = false;
  int dropped_frames = 0;
  dropped_frames += incomplete_frames_.RecycleFramesUntilKeyFrame(
      &key_frame_it, &free_frames_);
  key_frame_found = key_frame_it != incomplete_frames_.end();
  if (dropped_frames == 0) {
    dropped_frames += decodable_frames_.RecycleFramesUntilKeyFrame(
        &key_frame_it, &free_frames_);
    key_frame_found = key_frame_it != decodable_frames_.end();
  }
  drop_count_ += dropped_frames;
  TRACE_EVENT_INSTANT0("webrtc", "JB::RecycleFramesUntilKeyFrame");
  if (key_frame_found) {
    LOG(LS_INFO) << "Found key frame while dropping frames.";
    
    
    last_decoded_state_.Reset();
    DropPacketsFromNackList(EstimatedLowSequenceNumber(*key_frame_it->second));
  } else if (decodable_frames_.empty()) {
    
    
    last_decoded_state_.Reset();
    missing_sequence_numbers_.clear();
  }
  return key_frame_found;
}


void VCMJitterBuffer::CountFrame(const VCMFrameBuffer& frame) {
  if (!frame.GetCountedFrame()) {
    
    incoming_frame_count_++;
  }

  if (frame.FrameType() == kVideoFrameKey) {
    TRACE_EVENT_ASYNC_STEP0("webrtc", "Video",
                            frame.TimeStamp(), "KeyComplete");
  } else {
    TRACE_EVENT_ASYNC_STEP0("webrtc", "Video",
                            frame.TimeStamp(), "DeltaComplete");
  }

  
  
  if (frame.IsSessionComplete()) {
    ++receive_statistics_[frame.FrameType()];
  }
}

void VCMJitterBuffer::UpdateAveragePacketsPerFrame(int current_number_packets) {
  if (frame_counter_ > kFastConvergeThreshold) {
    average_packets_per_frame_ = average_packets_per_frame_
              * (1 - kNormalConvergeMultiplier)
            + current_number_packets * kNormalConvergeMultiplier;
  } else if (frame_counter_ > 0) {
    average_packets_per_frame_ = average_packets_per_frame_
              * (1 - kFastConvergeMultiplier)
            + current_number_packets * kFastConvergeMultiplier;
    frame_counter_++;
  } else {
    average_packets_per_frame_ = current_number_packets;
    frame_counter_++;
  }
}


void VCMJitterBuffer::CleanUpOldOrEmptyFrames() {
  drop_count_ +=
      decodable_frames_.CleanUpOldOrEmptyFrames(&last_decoded_state_,
          &free_frames_);
  drop_count_ +=
      incomplete_frames_.CleanUpOldOrEmptyFrames(&last_decoded_state_,
          &free_frames_);
  if (!last_decoded_state_.in_initial_state()) {
    DropPacketsFromNackList(last_decoded_state_.sequence_num());
  }
}


bool VCMJitterBuffer::IsPacketRetransmitted(const VCMPacket& packet) const {
  return missing_sequence_numbers_.find(packet.seqNum) !=
      missing_sequence_numbers_.end();
}




void VCMJitterBuffer::UpdateJitterEstimate(const VCMJitterSample& sample,
                                           bool incomplete_frame) {
  if (sample.latest_packet_time == -1) {
    return;
  }
  UpdateJitterEstimate(sample.latest_packet_time, sample.timestamp,
                       sample.frame_size, incomplete_frame);
}




void VCMJitterBuffer::UpdateJitterEstimate(const VCMFrameBuffer& frame,
                                           bool incomplete_frame) {
  if (frame.LatestPacketTimeMs() == -1) {
    return;
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
  bool not_reordered = inter_frame_delay_.CalculateDelay(timestamp,
                                                      &frame_delay,
                                                      latest_packet_time_ms);
  
  if (not_reordered) {
    
    jitter_estimate_.UpdateEstimate(frame_delay, frame_size, incomplete_frame);
  }
}

bool VCMJitterBuffer::WaitForRetransmissions() {
  if (nack_mode_ == kNoNack) {
    
    return false;
  }
  
  
  if (high_rtt_nack_threshold_ms_ >= 0 &&
      rtt_ms_ >= static_cast<unsigned int>(high_rtt_nack_threshold_ms_)) {
    return false;
  }
  return true;
}
}  
