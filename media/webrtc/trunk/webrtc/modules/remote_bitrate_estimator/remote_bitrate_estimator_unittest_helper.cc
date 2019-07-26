








#include "webrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_unittest_helper.h"

#include <algorithm>
#include <utility>

namespace webrtc {
namespace testing {

RtpStream::RtpStream(int fps,
                     int bitrate_bps,
                     unsigned int ssrc,
                     unsigned int frequency,
                     uint32_t timestamp_offset,
                     int64_t rtcp_receive_time)
    : fps_(fps),
      bitrate_bps_(bitrate_bps),
      ssrc_(ssrc),
      frequency_(frequency),
      next_rtp_time_(0),
      next_rtcp_time_(rtcp_receive_time),
      rtp_timestamp_offset_(timestamp_offset),
      kNtpFracPerMs(4.294967296E6) {
  assert(fps_ > 0);
}

void RtpStream::set_rtp_timestamp_offset(uint32_t offset) {
  rtp_timestamp_offset_ = offset;
}




int64_t RtpStream::GenerateFrame(double time_now, PacketList* packets) {
  if (time_now < next_rtp_time_) {
    return next_rtp_time_;
  }
  assert(packets != NULL);
  int bits_per_frame = (bitrate_bps_ + fps_ / 2) / fps_;
  int n_packets = std::max((bits_per_frame + 8 * kMtu) / (8 * kMtu), 1);
  int packet_size = (bits_per_frame + 4 * n_packets) / (8 * n_packets);
  assert(n_packets >= 0);
  for (int i = 0; i < n_packets; ++i) {
    RtpPacket* packet = new RtpPacket;
    packet->send_time = time_now + kSendSideOffsetMs + 0.5f;
    packet->size = packet_size;
    packet->rtp_timestamp = rtp_timestamp_offset_ + static_cast<uint32_t>(
        (frequency_ / 1000.0) * packet->send_time + 0.5);
    packet->ssrc = ssrc_;
    packets->push_back(packet);
  }
  next_rtp_time_ = time_now + 1000.0 / static_cast<double>(fps_);
  return next_rtp_time_;
}


double RtpStream::next_rtp_time() const {
  return next_rtp_time_;
}


RtpStream::RtcpPacket* RtpStream::Rtcp(double time_now) {
  if (time_now < next_rtcp_time_) {
    return NULL;
  }
  RtcpPacket* rtcp = new RtcpPacket;
  int64_t send_time = RtpStream::kSendSideOffsetMs + time_now + 0.5;
  rtcp->timestamp = rtp_timestamp_offset_ + static_cast<uint32_t>(
      (frequency_ / 1000.0) * send_time + 0.5);
  rtcp->ntp_secs = send_time / 1000;
  rtcp->ntp_frac = (send_time % 1000) * kNtpFracPerMs;
  rtcp->ssrc = ssrc_;
  next_rtcp_time_ = time_now + kRtcpIntervalMs;
  return rtcp;
}

void RtpStream::set_bitrate_bps(int bitrate_bps) {
  ASSERT_GE(bitrate_bps, 0);
  bitrate_bps_ = bitrate_bps;
}

int RtpStream::bitrate_bps() const {
  return bitrate_bps_;
}

unsigned int RtpStream::ssrc() const {
  return ssrc_;
}

bool RtpStream::Compare(const std::pair<unsigned int, RtpStream*>& left,
                        const std::pair<unsigned int, RtpStream*>& right) {
  return left.second->next_rtp_time_ < right.second->next_rtp_time_;
}

StreamGenerator::StreamGenerator(int capacity, double time_now)
    : capacity_(capacity),
      prev_arrival_time_(time_now) {}

StreamGenerator::~StreamGenerator() {
  for (StreamMap::iterator it = streams_.begin(); it != streams_.end();
      ++it) {
    delete it->second;
  }
  streams_.clear();
}


void StreamGenerator::AddStream(RtpStream* stream) {
  streams_[stream->ssrc()] = stream;
}


void StreamGenerator::set_capacity_bps(int capacity_bps) {
  ASSERT_GT(capacity_bps, 0);
  capacity_ = capacity_bps;
}



void StreamGenerator::set_bitrate_bps(int bitrate_bps) {
  ASSERT_GE(streams_.size(), 0u);
  double total_bitrate_before = 0;
  for (StreamMap::iterator it = streams_.begin(); it != streams_.end();
          ++it) {
    total_bitrate_before += it->second->bitrate_bps();
  }
  int total_bitrate_after = 0;
  for (StreamMap::iterator it = streams_.begin(); it != streams_.end();
      ++it) {
    double ratio = it->second->bitrate_bps() / total_bitrate_before;
    it->second->set_bitrate_bps(ratio * bitrate_bps + 0.5);
    total_bitrate_after += it->second->bitrate_bps();
  }
  EXPECT_NEAR(total_bitrate_after, bitrate_bps, 1);
}


void StreamGenerator::set_rtp_timestamp_offset(unsigned int ssrc,
                                               uint32_t offset) {
  streams_[ssrc]->set_rtp_timestamp_offset(offset);
}



double StreamGenerator::GenerateFrame(RtpStream::PacketList* packets,
                                      double time_now) {
  assert(packets != NULL);
  assert(packets->empty());
  assert(capacity_ > 0);
  StreamMap::iterator it = std::min_element(streams_.begin(), streams_.end(),
                                            RtpStream::Compare);
  (*it).second->GenerateFrame(time_now, packets);
  for (RtpStream::PacketList::iterator packet_it = packets->begin();
      packet_it != packets->end(); ++packet_it) {
    int required_network_time =
        (8 * 1000 * (*packet_it)->size + capacity_ / 2) / capacity_;
    prev_arrival_time_ = std::max(time_now + required_network_time,
        prev_arrival_time_ + required_network_time);
    (*packet_it)->arrival_time = prev_arrival_time_ + 0.5;
  }
  it = std::min_element(streams_.begin(), streams_.end(), RtpStream::Compare);
  return (*it).second->next_rtp_time();
}

void StreamGenerator::Rtcps(RtcpList* rtcps, double time_now) const {
  for (StreamMap::const_iterator it = streams_.begin(); it != streams_.end();
      ++it) {
    RtpStream::RtcpPacket* rtcp = it->second->Rtcp(time_now);
    if (rtcp) {
      rtcps->push_front(rtcp);
    }
  }
}
}  

RemoteBitrateEstimatorTest::RemoteBitrateEstimatorTest()
    : time_now_(0.0),
      align_streams_(false) {}

RemoteBitrateEstimatorTest::RemoteBitrateEstimatorTest(bool align_streams)
    : time_now_(0.0),
      align_streams_(align_streams) {}

void RemoteBitrateEstimatorTest::SetUp() {
  bitrate_observer_.reset(new testing::TestBitrateObserver);
  bitrate_estimator_.reset(
      RemoteBitrateEstimator::Create(
          bitrate_observer_.get(),
          over_use_detector_options_,
          RemoteBitrateEstimator::kSingleStreamEstimation));
  stream_generator_.reset(new testing::StreamGenerator(1e6,  
                                                       time_now_));
}

void RemoteBitrateEstimatorTest::AddDefaultStream() {
  stream_generator_->AddStream(new testing::RtpStream(
    30,          
    3e5,         
    1,           
    90000,       
    0xFFFFF000,  
    0));         
}







bool RemoteBitrateEstimatorTest::GenerateAndProcessFrame(unsigned int ssrc,
    unsigned int bitrate_bps) {
  stream_generator_->set_bitrate_bps(bitrate_bps);
  testing::RtpStream::PacketList packets;
  time_now_ = stream_generator_->GenerateFrame(&packets, time_now_);
  int64_t last_arrival_time = -1;
  bool prev_was_decrease = false;
  bool overuse = false;
  while (!packets.empty()) {
    testing::RtpStream::RtpPacket* packet = packets.front();
    if (align_streams_) {
      testing::StreamGenerator::RtcpList rtcps;
      stream_generator_->Rtcps(&rtcps, time_now_);
      for (testing::StreamGenerator::RtcpList::iterator it = rtcps.begin();
          it != rtcps.end(); ++it) {
        bitrate_estimator_->IncomingRtcp((*it)->ssrc,
                                         (*it)->ntp_secs,
                                         (*it)->ntp_frac,
                                         (*it)->timestamp);
        delete *it;
      }
    }
    bitrate_observer_->Reset();
    bitrate_estimator_->IncomingPacket(packet->ssrc,
                                       packet->size,
                                       packet->arrival_time,
                                       packet->rtp_timestamp);
    if (bitrate_observer_->updated()) {
      
      
      overuse = true;
      EXPECT_LE(bitrate_observer_->latest_bitrate(), bitrate_bps);
      EXPECT_FALSE(prev_was_decrease);
      prev_was_decrease = true;
    } else {
      prev_was_decrease = false;
    }
    last_arrival_time = packet->arrival_time;
    delete packet;
    packets.pop_front();
  }
  EXPECT_GT(last_arrival_time, -1);
  bitrate_estimator_->UpdateEstimate(ssrc, last_arrival_time);
  return overuse;
}




unsigned int RemoteBitrateEstimatorTest::SteadyStateRun(
    unsigned int ssrc,
    int number_of_frames,
    unsigned int start_bitrate,
    unsigned int min_bitrate,
    unsigned int max_bitrate) {
  unsigned int bitrate_bps = start_bitrate;
  bool bitrate_update_seen = false;
  
  for (int i = 0; i < number_of_frames; ++i) {
    bool overuse = GenerateAndProcessFrame(ssrc, bitrate_bps);
    if (overuse) {
      EXPECT_LT(bitrate_observer_->latest_bitrate(), max_bitrate);
      EXPECT_GT(bitrate_observer_->latest_bitrate(), min_bitrate);
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_update_seen = true;
    } else if (bitrate_observer_->updated()) {
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_TRUE(bitrate_update_seen);
  return bitrate_bps;
}
}  
