








#include "webrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_unittest_helper.h"

#include <algorithm>
#include <utility>

namespace webrtc {

enum { kMtu = 1200 };

namespace testing {

void TestBitrateObserver::OnReceiveBitrateChanged(
    const std::vector<unsigned int>& ssrcs,
    unsigned int bitrate) {
  latest_bitrate_ = bitrate;
  updated_ = true;
}

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




int64_t RtpStream::GenerateFrame(int64_t time_now_us, PacketList* packets) {
  if (time_now_us < next_rtp_time_) {
    return next_rtp_time_;
  }
  assert(packets != NULL);
  int bits_per_frame = (bitrate_bps_ + fps_ / 2) / fps_;
  int n_packets = std::max((bits_per_frame + 4 * kMtu) / (8 * kMtu), 1);
  int packet_size = (bits_per_frame + 4 * n_packets) / (8 * n_packets);
  assert(n_packets >= 0);
  for (int i = 0; i < n_packets; ++i) {
    RtpPacket* packet = new RtpPacket;
    packet->send_time = time_now_us + kSendSideOffsetUs;
    packet->size = packet_size;
    packet->rtp_timestamp = rtp_timestamp_offset_ + static_cast<uint32_t>(
        ((frequency_ / 1000) * packet->send_time + 500) / 1000);
    packet->ssrc = ssrc_;
    packets->push_back(packet);
  }
  next_rtp_time_ = time_now_us + (1000000 + fps_ / 2) / fps_;
  return next_rtp_time_;
}


double RtpStream::next_rtp_time() const {
  return next_rtp_time_;
}


RtpStream::RtcpPacket* RtpStream::Rtcp(int64_t time_now_us) {
  if (time_now_us < next_rtcp_time_) {
    return NULL;
  }
  RtcpPacket* rtcp = new RtcpPacket;
  int64_t send_time_us = time_now_us + kSendSideOffsetUs;
  rtcp->timestamp = rtp_timestamp_offset_ + static_cast<uint32_t>(
      ((frequency_ / 1000) * send_time_us + 500) / 1000);
  rtcp->ntp_secs = send_time_us / 1000000;
  rtcp->ntp_frac = static_cast<int64_t>((send_time_us % 1000000) *
      kNtpFracPerMs);
  rtcp->ssrc = ssrc_;
  next_rtcp_time_ = time_now_us + kRtcpIntervalUs;
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
      prev_arrival_time_us_(time_now) {}

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



void StreamGenerator::SetBitrateBps(int bitrate_bps) {
  ASSERT_GE(streams_.size(), 0u);
  int total_bitrate_before = 0;
  for (StreamMap::iterator it = streams_.begin(); it != streams_.end(); ++it) {
    total_bitrate_before += it->second->bitrate_bps();
  }
  int64_t bitrate_before = 0;
  int total_bitrate_after = 0;
  for (StreamMap::iterator it = streams_.begin(); it != streams_.end(); ++it) {
    bitrate_before += it->second->bitrate_bps();
    int64_t bitrate_after = (bitrate_before * bitrate_bps +
        total_bitrate_before / 2) / total_bitrate_before;
    it->second->set_bitrate_bps(bitrate_after - total_bitrate_after);
    total_bitrate_after += it->second->bitrate_bps();
  }
  ASSERT_EQ(bitrate_before, total_bitrate_before);
  EXPECT_EQ(total_bitrate_after, bitrate_bps);
}


void StreamGenerator::set_rtp_timestamp_offset(unsigned int ssrc,
                                               uint32_t offset) {
  streams_[ssrc]->set_rtp_timestamp_offset(offset);
}



int64_t StreamGenerator::GenerateFrame(RtpStream::PacketList* packets,
                                       int64_t time_now_us) {
  assert(packets != NULL);
  assert(packets->empty());
  assert(capacity_ > 0);
  StreamMap::iterator it = std::min_element(streams_.begin(), streams_.end(),
                                            RtpStream::Compare);
  (*it).second->GenerateFrame(time_now_us, packets);
  int i = 0;
  for (RtpStream::PacketList::iterator packet_it = packets->begin();
      packet_it != packets->end(); ++packet_it) {
    int capacity_bpus = capacity_ / 1000;
    int64_t required_network_time_us =
        (8 * 1000 * (*packet_it)->size + capacity_bpus / 2) / capacity_bpus;
    prev_arrival_time_us_ = std::max(time_now_us + required_network_time_us,
        prev_arrival_time_us_ + required_network_time_us);
    (*packet_it)->arrival_time = prev_arrival_time_us_;
    ++i;
  }
  it = std::min_element(streams_.begin(), streams_.end(), RtpStream::Compare);
  return (*it).second->next_rtp_time();
}
}  

RemoteBitrateEstimatorTest::RemoteBitrateEstimatorTest()
    : clock_(0),
      bitrate_observer_(new testing::TestBitrateObserver),
      stream_generator_(new testing::StreamGenerator(
          1e6,  
          clock_.TimeInMicroseconds())) {}

RemoteBitrateEstimatorTest::~RemoteBitrateEstimatorTest() {}

void RemoteBitrateEstimatorTest::AddDefaultStream() {
  stream_generator_->AddStream(new testing::RtpStream(
    30,          
    3e5,         
    1,           
    90000,       
    0xFFFFF000,  
    0));         
}

uint32_t RemoteBitrateEstimatorTest::AbsSendTime(int64_t t, int64_t denom) {
  return (((t << 18) + (denom >> 1)) / denom) & 0x00fffffful;
}

uint32_t RemoteBitrateEstimatorTest::AddAbsSendTime(uint32_t t1, uint32_t t2) {
  return (t1 + t2) & 0x00fffffful;
}

const unsigned int RemoteBitrateEstimatorTest::kDefaultSsrc = 1;

void RemoteBitrateEstimatorTest::IncomingPacket(uint32_t ssrc,
                                                uint32_t payload_size,
                                                int64_t arrival_time,
                                                uint32_t rtp_timestamp,
                                                uint32_t absolute_send_time) {
  RTPHeader header;
  memset(&header, 0, sizeof(header));
  header.ssrc = ssrc;
  header.timestamp = rtp_timestamp;
  header.extension.absoluteSendTime = absolute_send_time;
  bitrate_estimator_->IncomingPacket(arrival_time, payload_size, header);
}







bool RemoteBitrateEstimatorTest::GenerateAndProcessFrame(unsigned int ssrc,
    unsigned int bitrate_bps) {
  stream_generator_->SetBitrateBps(bitrate_bps);
  testing::RtpStream::PacketList packets;
  int64_t next_time_us = stream_generator_->GenerateFrame(
      &packets, clock_.TimeInMicroseconds());
  bool overuse = false;
  while (!packets.empty()) {
    testing::RtpStream::RtpPacket* packet = packets.front();
    bitrate_observer_->Reset();
    IncomingPacket(packet->ssrc,
                   packet->size,
                   (packet->arrival_time + 500) / 1000,
                   packet->rtp_timestamp,
                   AbsSendTime(packet->send_time, 1000000));
    if (bitrate_observer_->updated()) {
      
      
      overuse = true;
      EXPECT_LE(bitrate_observer_->latest_bitrate(), bitrate_bps);
    }
    clock_.AdvanceTimeMicroseconds(packet->arrival_time -
                                   clock_.TimeInMicroseconds());
    delete packet;
    packets.pop_front();
  }
  bitrate_estimator_->Process();
  clock_.AdvanceTimeMicroseconds(next_time_us - clock_.TimeInMicroseconds());
  return overuse;
}





unsigned int RemoteBitrateEstimatorTest::SteadyStateRun(
    unsigned int ssrc,
    int max_number_of_frames,
    unsigned int start_bitrate,
    unsigned int min_bitrate,
    unsigned int max_bitrate,
    unsigned int target_bitrate) {
  unsigned int bitrate_bps = start_bitrate;
  bool bitrate_update_seen = false;
  
  for (int i = 0; i < max_number_of_frames; ++i) {
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
    if (bitrate_update_seen && bitrate_bps > target_bitrate) {
      break;
    }
  }
  EXPECT_TRUE(bitrate_update_seen);
  return bitrate_bps;
}

void RemoteBitrateEstimatorTest::InitialBehaviorTestHelper(
    unsigned int expected_converge_bitrate) {
  const int kFramerate = 50;  
  const int kFrameIntervalMs = 1000 / kFramerate;
  const uint32_t kFrameIntervalAbsSendTime = AbsSendTime(1, kFramerate);
  unsigned int bitrate_bps = 0;
  uint32_t timestamp = 0;
  uint32_t absolute_send_time = 0;
  std::vector<unsigned int> ssrcs;
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_EQ(0u, ssrcs.size());
  clock_.AdvanceTimeMilliseconds(1000);
  bitrate_estimator_->Process();
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_FALSE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  clock_.AdvanceTimeMilliseconds(1000);
  
  IncomingPacket(kDefaultSsrc, kMtu, clock_.TimeInMilliseconds(), timestamp,
                 absolute_send_time);
  bitrate_estimator_->Process();
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_EQ(0u, ssrcs.size());
  EXPECT_FALSE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  
  for (int i = 0; i < kFramerate; ++i) {
    IncomingPacket(kDefaultSsrc, kMtu, clock_.TimeInMilliseconds(), timestamp,
                   absolute_send_time);
    clock_.AdvanceTimeMilliseconds(1000 / kFramerate);
    timestamp += 90 * kFrameIntervalMs;
    absolute_send_time = AddAbsSendTime(absolute_send_time,
                                        kFrameIntervalAbsSendTime);
  }
  bitrate_estimator_->Process();
  EXPECT_TRUE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  ASSERT_EQ(1u, ssrcs.size());
  EXPECT_EQ(kDefaultSsrc, ssrcs.front());
  EXPECT_EQ(expected_converge_bitrate, bitrate_bps);
  EXPECT_TRUE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  EXPECT_EQ(bitrate_observer_->latest_bitrate(), bitrate_bps);
}

void RemoteBitrateEstimatorTest::RateIncreaseReorderingTestHelper() {
  const int kFramerate = 50;  
  const int kFrameIntervalMs = 1000 / kFramerate;
  const uint32_t kFrameIntervalAbsSendTime = AbsSendTime(1, kFramerate);
  uint32_t timestamp = 0;
  uint32_t absolute_send_time = 0;
  IncomingPacket(kDefaultSsrc, 1000, clock_.TimeInMilliseconds(), timestamp,
                 absolute_send_time);
  bitrate_estimator_->Process();
  EXPECT_FALSE(bitrate_observer_->updated());  
  
  for (int i = 0; i < kFramerate; ++i) {
    IncomingPacket(kDefaultSsrc, kMtu, clock_.TimeInMilliseconds(), timestamp,
                   absolute_send_time);
    clock_.AdvanceTimeMilliseconds(kFrameIntervalMs);
    timestamp += 90 * kFrameIntervalMs;
    absolute_send_time = AddAbsSendTime(absolute_send_time,
                                        kFrameIntervalAbsSendTime);
  }
  bitrate_estimator_->Process();
  EXPECT_TRUE(bitrate_observer_->updated());
  EXPECT_EQ(498136u, bitrate_observer_->latest_bitrate());
  for (int i = 0; i < 10; ++i) {
    clock_.AdvanceTimeMilliseconds(2 * kFrameIntervalMs);
    timestamp += 2 * 90 * kFrameIntervalMs;
    absolute_send_time = AddAbsSendTime(absolute_send_time,
                                        2 * kFrameIntervalAbsSendTime);
    IncomingPacket(kDefaultSsrc, 1000, clock_.TimeInMilliseconds(), timestamp,
                   absolute_send_time);
    IncomingPacket(kDefaultSsrc, 1000, clock_.TimeInMilliseconds(),
                   timestamp - 90 * kFrameIntervalMs,
                   AddAbsSendTime(absolute_send_time,
                                  -int(kFrameIntervalAbsSendTime)));
  }
  bitrate_estimator_->Process();
  EXPECT_TRUE(bitrate_observer_->updated());
  EXPECT_EQ(498136u, bitrate_observer_->latest_bitrate());
}


void RemoteBitrateEstimatorTest::RateIncreaseRtpTimestampsTestHelper() {
  
  
  
  const int kExpectedIterations = 1621;
  unsigned int bitrate_bps = 30000;
  int iterations = 0;
  AddDefaultStream();
  
  
  while (bitrate_bps < 5e5) {
    bool overuse = GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    if (overuse) {
      EXPECT_GT(bitrate_observer_->latest_bitrate(), bitrate_bps);
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    } else if (bitrate_observer_->updated()) {
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
    ++iterations;
    ASSERT_LE(iterations, kExpectedIterations);
  }
  ASSERT_EQ(kExpectedIterations, iterations);
}

void RemoteBitrateEstimatorTest::CapacityDropTestHelper(
    int number_of_streams,
    bool wrap_time_stamp,
    unsigned int expected_converge_bitrate,
    unsigned int expected_bitrate_drop_delta) {
  const int kFramerate = 30;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  const unsigned int kInitialCapacityBps = 1000e3;
  const unsigned int kReducedCapacityBps = 500e3;

  int steady_state_time = 0;
  int expected_overuse_start_time = 0;
  if (number_of_streams <= 1) {
    steady_state_time = 10;
    expected_overuse_start_time = 10000;
    AddDefaultStream();
  } else {
    steady_state_time = 8 * number_of_streams;
    expected_overuse_start_time = 8000;
    int bitrate_sum = 0;
    int kBitrateDenom = number_of_streams * (number_of_streams - 1);
    for (int i = 0; i < number_of_streams; i++) {
      
      
      int bitrate = kStartBitrate / 2;
      if (i > 0) {
        bitrate = (kStartBitrate * i + kBitrateDenom / 2) / kBitrateDenom;
      }
      stream_generator_->AddStream(new testing::RtpStream(
          kFramerate,                     
          bitrate,                        
          kDefaultSsrc + i,               
          90000,                          
          0xFFFFF000 ^ (~0 << (32 - i)),  
          0));                            
      bitrate_sum += bitrate;
    }
    ASSERT_EQ(bitrate_sum, kStartBitrate);
  }
  if (wrap_time_stamp) {
    stream_generator_->set_rtp_timestamp_offset(kDefaultSsrc,
        std::numeric_limits<uint32_t>::max() - steady_state_time * 90000);
  }

  
  stream_generator_->set_capacity_bps(kInitialCapacityBps);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            steady_state_time * kFramerate,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate,
                                            kInitialCapacityBps);
  EXPECT_EQ(expected_converge_bitrate, bitrate_bps);
  bitrate_observer_->Reset();

  
  stream_generator_->set_capacity_bps(kReducedCapacityBps);
  int64_t overuse_start_time = clock_.TimeInMilliseconds();
  EXPECT_EQ(expected_overuse_start_time, overuse_start_time);
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 100 * number_of_streams; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= kReducedCapacityBps) {
        bitrate_drop_time = clock_.TimeInMilliseconds();
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }

  EXPECT_EQ(expected_bitrate_drop_delta,
            bitrate_drop_time - overuse_start_time);
}
}  
