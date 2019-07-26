









#include "webrtc/modules/rtp_rtcp/source/receive_statistics_impl.h"

#include <math.h>

#include "webrtc/modules/rtp_rtcp/source/bitrate.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

const int64_t kStatisticsTimeoutMs = 8000;
const int kStatisticsProcessIntervalMs = 1000;

StreamStatistician::~StreamStatistician() {}

StreamStatisticianImpl::StreamStatisticianImpl(Clock* clock)
    : clock_(clock),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      incoming_bitrate_(clock),
      ssrc_(0),
      max_reordering_threshold_(kDefaultMaxReorderingThreshold),
      jitter_q4_(0),
      jitter_max_q4_(0),
      cumulative_loss_(0),
      jitter_q4_transmission_time_offset_(0),
      last_receive_time_ms_(0),
      last_receive_time_secs_(0),
      last_receive_time_frac_(0),
      last_received_timestamp_(0),
      last_received_transmission_time_offset_(0),
      received_seq_first_(0),
      received_seq_max_(0),
      received_seq_wraps_(0),
      first_packet_(true),
      received_packet_overhead_(12),
      received_byte_count_(0),
      received_retransmitted_packets_(0),
      received_inorder_packet_count_(0),
      last_report_inorder_packets_(0),
      last_report_old_packets_(0),
      last_report_seq_max_(0),
      last_reported_statistics_() {}

void StreamStatisticianImpl::ResetStatistics() {
  CriticalSectionScoped cs(crit_sect_.get());
  last_report_inorder_packets_ = 0;
  last_report_old_packets_ = 0;
  last_report_seq_max_ = 0;
  memset(&last_reported_statistics_, 0, sizeof(last_reported_statistics_));
  jitter_q4_ = 0;
  jitter_max_q4_ = 0;
  cumulative_loss_ = 0;
  jitter_q4_transmission_time_offset_ = 0;
  received_seq_wraps_ = 0;
  received_seq_max_ = 0;
  received_seq_first_ = 0;
  received_byte_count_ = 0;
  received_retransmitted_packets_ = 0;
  received_inorder_packet_count_ = 0;
  first_packet_ = true;
}

void StreamStatisticianImpl::IncomingPacket(const RTPHeader& header,
                                            size_t bytes,
                                            bool retransmitted) {
  CriticalSectionScoped cs(crit_sect_.get());
  bool in_order = InOrderPacketInternal(header.sequenceNumber);
  ssrc_ = header.ssrc;
  incoming_bitrate_.Update(bytes);
  received_byte_count_ += bytes;

  if (first_packet_) {
    first_packet_ = false;
    
    received_seq_first_ = header.sequenceNumber;
    received_seq_max_ = header.sequenceNumber;
    received_inorder_packet_count_ = 1;
    clock_->CurrentNtp(last_receive_time_secs_, last_receive_time_frac_);
    last_receive_time_ms_ = clock_->TimeInMilliseconds();
    return;
  }

  
  
  if (in_order) {
    
    uint32_t receive_time_secs;
    uint32_t receive_time_frac;
    clock_->CurrentNtp(receive_time_secs, receive_time_frac);
    received_inorder_packet_count_++;

    
    int32_t seq_diff = header.sequenceNumber - received_seq_max_;
    if (seq_diff < 0) {
      
      received_seq_wraps_++;
    }
    
    received_seq_max_ = header.sequenceNumber;

    if (header.timestamp != last_received_timestamp_ &&
        received_inorder_packet_count_ > 1) {
      uint32_t receive_time_rtp = ModuleRTPUtility::ConvertNTPTimeToRTP(
          receive_time_secs, receive_time_frac, header.payload_type_frequency);
      uint32_t last_receive_time_rtp = ModuleRTPUtility::ConvertNTPTimeToRTP(
          last_receive_time_secs_, last_receive_time_frac_,
          header.payload_type_frequency);
      int32_t time_diff_samples = (receive_time_rtp - last_receive_time_rtp) -
          (header.timestamp - last_received_timestamp_);

      time_diff_samples = abs(time_diff_samples);

      
      
      
      if (time_diff_samples < 450000) {
        
        int32_t jitter_diff_q4 = (time_diff_samples << 4) - jitter_q4_;
        jitter_q4_ += ((jitter_diff_q4 + 8) >> 4);
      }

      
      
      int32_t time_diff_samples_ext =
        (receive_time_rtp - last_receive_time_rtp) -
        ((header.timestamp +
          header.extension.transmissionTimeOffset) -
         (last_received_timestamp_ +
          last_received_transmission_time_offset_));

      time_diff_samples_ext = abs(time_diff_samples_ext);

      if (time_diff_samples_ext < 450000) {
        int32_t jitter_diffQ4TransmissionTimeOffset =
          (time_diff_samples_ext << 4) - jitter_q4_transmission_time_offset_;
        jitter_q4_transmission_time_offset_ +=
          ((jitter_diffQ4TransmissionTimeOffset + 8) >> 4);
      }
    }
    last_received_timestamp_ = header.timestamp;
    last_receive_time_secs_ = receive_time_secs;
    last_receive_time_frac_ = receive_time_frac;
    last_receive_time_ms_ = clock_->TimeInMilliseconds();
  } else {
    if (retransmitted) {
      received_retransmitted_packets_++;
    } else {
      received_inorder_packet_count_++;
    }
  }

  uint16_t packet_oh = header.headerLength + header.paddingLength;

  
  
  received_packet_overhead_ = (15 * received_packet_overhead_ + packet_oh) >> 4;
}

void StreamStatisticianImpl::SetMaxReorderingThreshold(
    int max_reordering_threshold) {
  CriticalSectionScoped cs(crit_sect_.get());
  max_reordering_threshold_ = max_reordering_threshold;
}

bool StreamStatisticianImpl::GetStatistics(Statistics* statistics, bool reset) {
  CriticalSectionScoped cs(crit_sect_.get());
  if (received_seq_first_ == 0 && received_byte_count_ == 0) {
    
    return false;
  }

  if (!reset) {
    if (last_report_inorder_packets_ == 0) {
      
      return false;
    }
    
    *statistics = last_reported_statistics_;
    return true;
  }

  if (last_report_inorder_packets_ == 0) {
    
    last_report_seq_max_ = received_seq_first_ - 1;
  }

  
  uint16_t exp_since_last = (received_seq_max_ - last_report_seq_max_);

  if (last_report_seq_max_ > received_seq_max_) {
    
    exp_since_last = 0;
  }

  
  
  uint32_t rec_since_last =
      received_inorder_packet_count_ - last_report_inorder_packets_;

  
  
  
  
  

  
  
  
  uint32_t retransmitted_packets =
      received_retransmitted_packets_ - last_report_old_packets_;
  rec_since_last += retransmitted_packets;

  int32_t missing = 0;
  if (exp_since_last > rec_since_last) {
    missing = (exp_since_last - rec_since_last);
  }
  uint8_t local_fraction_lost = 0;
  if (exp_since_last) {
    
    local_fraction_lost =
        static_cast<uint8_t>(255 * missing / exp_since_last);
  }
  statistics->fraction_lost = local_fraction_lost;

  
  cumulative_loss_ += missing;

  if (jitter_q4_ > jitter_max_q4_) {
    jitter_max_q4_ = jitter_q4_;
  }
  statistics->cumulative_lost = cumulative_loss_;
  statistics->extended_max_sequence_number = (received_seq_wraps_ << 16) +
      received_seq_max_;
  
  statistics->jitter = jitter_q4_ >> 4;
  statistics->max_jitter = jitter_max_q4_ >> 4;
  if (reset) {
    
    last_reported_statistics_ = *statistics;

    
    last_report_inorder_packets_ = received_inorder_packet_count_;
    last_report_old_packets_ = received_retransmitted_packets_;
    last_report_seq_max_ = received_seq_max_;
  }
  return true;
}

void StreamStatisticianImpl::GetDataCounters(
    uint32_t* bytes_received, uint32_t* packets_received) const {
  CriticalSectionScoped cs(crit_sect_.get());
  if (bytes_received) {
    *bytes_received = received_byte_count_;
  }
  if (packets_received) {
    *packets_received =
        received_retransmitted_packets_ + received_inorder_packet_count_;
  }
}

uint32_t StreamStatisticianImpl::BitrateReceived() const {
  CriticalSectionScoped cs(crit_sect_.get());
  return incoming_bitrate_.BitrateNow();
}

void StreamStatisticianImpl::ProcessBitrate() {
  CriticalSectionScoped cs(crit_sect_.get());
  incoming_bitrate_.Process();
}

void StreamStatisticianImpl::LastReceiveTimeNtp(uint32_t* secs,
                                                uint32_t* frac) const {
  CriticalSectionScoped cs(crit_sect_.get());
  *secs = last_receive_time_secs_;
  *frac = last_receive_time_frac_;
}

bool StreamStatisticianImpl::IsRetransmitOfOldPacket(
    const RTPHeader& header, int min_rtt) const {
  CriticalSectionScoped cs(crit_sect_.get());
  if (InOrderPacketInternal(header.sequenceNumber)) {
    return false;
  }
  uint32_t frequency_khz = header.payload_type_frequency / 1000;
  assert(frequency_khz > 0);

  int64_t time_diff_ms = clock_->TimeInMilliseconds() -
      last_receive_time_ms_;

  
  uint32_t timestamp_diff = header.timestamp - last_received_timestamp_;
  int32_t rtp_time_stamp_diff_ms = static_cast<int32_t>(timestamp_diff) /
      frequency_khz;

  int32_t max_delay_ms = 0;
  if (min_rtt == 0) {
    
    float jitter_std = sqrt(static_cast<float>(jitter_q4_ >> 4));

    
    
    max_delay_ms = static_cast<int32_t>((2 * jitter_std) / frequency_khz);

    
    if (max_delay_ms == 0) {
      max_delay_ms = 1;
    }
  } else {
    max_delay_ms = (min_rtt / 3) + 1;
  }
  return time_diff_ms > rtp_time_stamp_diff_ms + max_delay_ms;
}

bool StreamStatisticianImpl::IsPacketInOrder(uint16_t sequence_number) const {
  CriticalSectionScoped cs(crit_sect_.get());
  return InOrderPacketInternal(sequence_number);
}

bool StreamStatisticianImpl::InOrderPacketInternal(
    uint16_t sequence_number) const {
  
  if (last_receive_time_ms_ == 0)
    return true;

  if (IsNewerSequenceNumber(sequence_number, received_seq_max_)) {
    return true;
  } else {
    
    return !IsNewerSequenceNumber(sequence_number, received_seq_max_ -
                                  max_reordering_threshold_);
  }
}

ReceiveStatistics* ReceiveStatistics::Create(Clock* clock) {
  return new ReceiveStatisticsImpl(clock);
}

ReceiveStatisticsImpl::ReceiveStatisticsImpl(Clock* clock)
    : clock_(clock),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      last_rate_update_ms_(0) {}

ReceiveStatisticsImpl::~ReceiveStatisticsImpl() {
  while (!statisticians_.empty()) {
    delete statisticians_.begin()->second;
    statisticians_.erase(statisticians_.begin());
  }
}

void ReceiveStatisticsImpl::IncomingPacket(const RTPHeader& header,
                                           size_t bytes, bool old_packet) {
  CriticalSectionScoped cs(crit_sect_.get());
  StatisticianImplMap::iterator it = statisticians_.find(header.ssrc);
  if (it == statisticians_.end()) {
    std::pair<StatisticianImplMap::iterator, uint32_t> insert_result =
        statisticians_.insert(std::make_pair(
            header.ssrc,  new StreamStatisticianImpl(clock_)));
    it = insert_result.first;
  }
  statisticians_[header.ssrc]->IncomingPacket(header, bytes, old_packet);
}

void ReceiveStatisticsImpl::ChangeSsrc(uint32_t from_ssrc, uint32_t to_ssrc) {
  CriticalSectionScoped cs(crit_sect_.get());
  StatisticianImplMap::iterator from_it = statisticians_.find(from_ssrc);
  if (from_it == statisticians_.end())
    return;
  if (statisticians_.find(to_ssrc) != statisticians_.end())
    return;
  statisticians_[to_ssrc] = from_it->second;
  statisticians_.erase(from_it);
}

StatisticianMap ReceiveStatisticsImpl::GetActiveStatisticians() const {
  CriticalSectionScoped cs(crit_sect_.get());
  StatisticianMap active_statisticians;
  for (StatisticianImplMap::const_iterator it = statisticians_.begin();
       it != statisticians_.end(); ++it) {
    uint32_t secs;
    uint32_t frac;
    it->second->LastReceiveTimeNtp(&secs, &frac);
    if (clock_->CurrentNtpInMilliseconds() -
        Clock::NtpToMs(secs, frac) < kStatisticsTimeoutMs) {
      active_statisticians[it->first] = it->second;
    }
  }
  return active_statisticians;
}

StreamStatistician* ReceiveStatisticsImpl::GetStatistician(
    uint32_t ssrc) const {
  CriticalSectionScoped cs(crit_sect_.get());
  StatisticianImplMap::const_iterator it = statisticians_.find(ssrc);
  if (it == statisticians_.end())
    return NULL;
  return it->second;
}

void ReceiveStatisticsImpl::SetMaxReorderingThreshold(
    int max_reordering_threshold) {
  CriticalSectionScoped cs(crit_sect_.get());
  for (StatisticianImplMap::iterator it = statisticians_.begin();
       it != statisticians_.end(); ++it) {
    it->second->SetMaxReorderingThreshold(max_reordering_threshold);
  }
}

int32_t ReceiveStatisticsImpl::Process() {
  CriticalSectionScoped cs(crit_sect_.get());
  for (StatisticianImplMap::iterator it = statisticians_.begin();
       it != statisticians_.end(); ++it) {
    it->second->ProcessBitrate();
  }
  last_rate_update_ms_ = clock_->TimeInMilliseconds();
  return 0;
}

int32_t ReceiveStatisticsImpl::TimeUntilNextProcess() {
  CriticalSectionScoped cs(crit_sect_.get());
  int time_since_last_update = clock_->TimeInMilliseconds() -
      last_rate_update_ms_;
  return std::max(kStatisticsProcessIntervalMs - time_since_last_update, 0);
}


void NullReceiveStatistics::IncomingPacket(const RTPHeader& rtp_header,
                                           size_t bytes,
                                           bool retransmitted) {}

StatisticianMap NullReceiveStatistics::GetActiveStatisticians() const {
  return StatisticianMap();
}

StreamStatistician* NullReceiveStatistics::GetStatistician(
    uint32_t ssrc) const {
  return NULL;
}

void NullReceiveStatistics::SetMaxReorderingThreshold(
    int max_reordering_threshold) {}

int32_t NullReceiveStatistics::TimeUntilNextProcess() { return 0; }

int32_t NullReceiveStatistics::Process() { return 0; }

}  
