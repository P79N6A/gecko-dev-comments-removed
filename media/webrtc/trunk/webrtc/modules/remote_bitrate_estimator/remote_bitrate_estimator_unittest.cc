











#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "modules/remote_bitrate_estimator/remote_bitrate_estimator_unittest_helper.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

TEST_F(RemoteBitrateEstimatorTest, TestInitialBehavior) {
  const int kFramerate = 50;  
  const int kFrameIntervalMs = 1000 / kFramerate;
  unsigned int bitrate_bps = 0;
  uint32_t timestamp = 0;
  std::vector<unsigned int> ssrcs;
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_EQ(0u, ssrcs.size());
  clock_.AdvanceTimeMilliseconds(1000);
  bitrate_estimator_->Process();
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_FALSE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  clock_.AdvanceTimeMilliseconds(1000);
  
  bitrate_estimator_->IncomingPacket(kDefaultSsrc, kMtu,
                                     clock_.TimeInMilliseconds(), timestamp);
  bitrate_estimator_->Process();
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_EQ(0u, ssrcs.size());
  EXPECT_FALSE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  
  for (int i = 0; i < kFramerate; ++i) {
    bitrate_estimator_->IncomingPacket(kDefaultSsrc, kMtu,
                                       clock_.TimeInMilliseconds(), timestamp);
    clock_.AdvanceTimeMilliseconds(1000 / kFramerate);
    timestamp += 90 * kFrameIntervalMs;
  }
  bitrate_estimator_->Process();
  EXPECT_TRUE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  ASSERT_EQ(1u, ssrcs.size());
  EXPECT_EQ(kDefaultSsrc, ssrcs.front());
  EXPECT_EQ(498075u, bitrate_bps);
  EXPECT_TRUE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  EXPECT_EQ(bitrate_observer_->latest_bitrate(), bitrate_bps);
}

TEST_F(RemoteBitrateEstimatorTest, TestRateIncreaseReordering) {
  uint32_t timestamp = 0;
  const int kFramerate = 50;  
  const int kFrameIntervalMs = 1000 / kFramerate;
  bitrate_estimator_->IncomingPacket(kDefaultSsrc, 1000,
                                     clock_.TimeInMilliseconds(), timestamp);
  bitrate_estimator_->Process();
  EXPECT_FALSE(bitrate_observer_->updated());  
  
  for (int i = 0; i < kFramerate; ++i) {
    bitrate_estimator_->IncomingPacket(kDefaultSsrc, kMtu,
                                       clock_.TimeInMilliseconds(), timestamp);
    clock_.AdvanceTimeMilliseconds(kFrameIntervalMs);
    timestamp += 90 * kFrameIntervalMs;
  }
  bitrate_estimator_->Process();
  EXPECT_TRUE(bitrate_observer_->updated());
  EXPECT_EQ(498136u, bitrate_observer_->latest_bitrate());
  for (int i = 0; i < 10; ++i) {
    clock_.AdvanceTimeMilliseconds(2 * kFrameIntervalMs);
    timestamp += 2 * 90 * kFrameIntervalMs;
    bitrate_estimator_->IncomingPacket(kDefaultSsrc, 1000,
                                       clock_.TimeInMilliseconds(), timestamp);
    bitrate_estimator_->IncomingPacket(kDefaultSsrc,
                                       1000,
                                       clock_.TimeInMilliseconds() -
                                           kFrameIntervalMs,
                                       timestamp - 90 * kFrameIntervalMs);
  }
  bitrate_estimator_->Process();
  EXPECT_TRUE(bitrate_observer_->updated());
  EXPECT_EQ(498136u, bitrate_observer_->latest_bitrate());
}


TEST_F(RemoteBitrateEstimatorTest, TestRateIncreaseRtpTimestamps) {
  
  
  
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



TEST_F(RemoteBitrateEstimatorTest, TestCapacityDropRtpTimestamps) {
  const int kNumberOfFrames = 300;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  AddDefaultStream();
  
  unsigned int capacity_bps = 1000e3;
  stream_generator_->set_capacity_bps(1000e3);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc, kNumberOfFrames,
                                            kStartBitrate, kMinExpectedBitrate,
                                            kMaxExpectedBitrate, capacity_bps);
  
  capacity_bps = 500e3;
  stream_generator_->set_capacity_bps(capacity_bps);
  int64_t overuse_start_time = clock_.TimeInMilliseconds();
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= capacity_bps) {
        bitrate_drop_time = clock_.TimeInMilliseconds();
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(367, bitrate_drop_time - overuse_start_time);
}




TEST_F(RemoteBitrateEstimatorTest, TestCapacityDropRtpTimestampsWrap) {
  const int kFramerate= 30;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  const int kSteadyStateTime = 8;  
  AddDefaultStream();
  
  stream_generator_->set_rtp_timestamp_offset(kDefaultSsrc,
      std::numeric_limits<uint32_t>::max() - kSteadyStateTime * 90000);
  
  unsigned int capacity_bps = 1000e3;
  stream_generator_->set_capacity_bps(1000e3);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            kSteadyStateTime * kFramerate,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate,
                                            capacity_bps);
  bitrate_observer_->Reset();
  
  capacity_bps = 500e3;
  stream_generator_->set_capacity_bps(capacity_bps);
  int64_t overuse_start_time = clock_.TimeInMilliseconds();
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= capacity_bps) {
        bitrate_drop_time = clock_.TimeInMilliseconds();
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(367, bitrate_drop_time - overuse_start_time);
}





TEST_F(RemoteBitrateEstimatorTestAlign, TestCapacityDropRtpTimestampsWrap) {
  const int kFramerate= 30;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  const int kSteadyStateTime = 8;  
  AddDefaultStream();
  
  stream_generator_->set_rtp_timestamp_offset(kDefaultSsrc,
      std::numeric_limits<uint32_t>::max() - kSteadyStateTime * 90000);
  
  unsigned int capacity_bps = 1000e3;
  stream_generator_->set_capacity_bps(capacity_bps);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            kSteadyStateTime * kFramerate,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate,
                                            capacity_bps);
  bitrate_observer_->Reset();
  
  capacity_bps = 500e3;
  stream_generator_->set_capacity_bps(capacity_bps);
  int64_t overuse_start_time = clock_.TimeInMilliseconds();
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= capacity_bps) {
        bitrate_drop_time = clock_.TimeInMilliseconds();
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(367, bitrate_drop_time - overuse_start_time);
}




TEST_F(RemoteBitrateEstimatorTestAlign, TwoStreamsCapacityDropWithWrap) {
  const int kFramerate= 30;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  const int kSteadyStateFrames = 9 * kFramerate;
  stream_generator_->AddStream(new testing::RtpStream(
      30,               
      kStartBitrate/2,  
      1,           
      90000,       
      0xFFFFF000,  
      0));         

  stream_generator_->AddStream(new testing::RtpStream(
      15,               
      kStartBitrate/2,  
      2,           
      90000,       
      0x00000FFF,  
      0));         
  
  stream_generator_->set_rtp_timestamp_offset(kDefaultSsrc,
      std::numeric_limits<uint32_t>::max() - kSteadyStateFrames * 90000);
  
  unsigned int capacity_bps = 1000e3;
  stream_generator_->set_capacity_bps(capacity_bps);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            kSteadyStateFrames,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate,
                                            capacity_bps);
  bitrate_observer_->Reset();
  
  capacity_bps = 500e3;
  stream_generator_->set_capacity_bps(capacity_bps);
  int64_t overuse_start_time = clock_.TimeInMilliseconds();
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= capacity_bps) {
        bitrate_drop_time = clock_.TimeInMilliseconds();
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(567, bitrate_drop_time - overuse_start_time);
}




TEST_F(RemoteBitrateEstimatorTestAlign, ThreeStreams) {
  const int kFramerate= 30;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  const int kSteadyStateFrames = 12 * kFramerate;
  stream_generator_->AddStream(new testing::RtpStream(
      kFramerate,       
      kStartBitrate/2,  
      1,            
      90000,        
      0xFFFFF000,   
      0));          

  stream_generator_->AddStream(new testing::RtpStream(
      kFramerate,       
      kStartBitrate/3,  
      2,            
      90000,        
      0x00000FFF,   
      0));          

  stream_generator_->AddStream(new testing::RtpStream(
      kFramerate,       
      kStartBitrate/6,  
      3,            
      90000,        
      0x00000FFF,   
      0));          
  
  stream_generator_->set_rtp_timestamp_offset(kDefaultSsrc,
      std::numeric_limits<uint32_t>::max() - kSteadyStateFrames * 90000);
  
  unsigned int capacity_bps = 1000e3;
  stream_generator_->set_capacity_bps(capacity_bps);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            kSteadyStateFrames,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate,
                                            capacity_bps);
  bitrate_observer_->Reset();
  
  capacity_bps = 500e3;
  stream_generator_->set_capacity_bps(capacity_bps);
  int64_t overuse_start_time = clock_.TimeInMilliseconds();
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= capacity_bps) {
        bitrate_drop_time = clock_.TimeInMilliseconds();
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(433, bitrate_drop_time - overuse_start_time);
}

}  
