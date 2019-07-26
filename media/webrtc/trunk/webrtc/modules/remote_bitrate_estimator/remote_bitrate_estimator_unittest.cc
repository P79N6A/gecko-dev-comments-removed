











#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "modules/remote_bitrate_estimator/remote_bitrate_estimator_unittest_helper.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

TEST_F(RemoteBitrateEstimatorTest, TestInitialBehavior) {
  unsigned int bitrate_bps = 0;
  int64_t time_now = 0;
  uint32_t timestamp = 0;
  std::vector<unsigned int> ssrcs;
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_EQ(0u, ssrcs.size());
  bitrate_estimator_->UpdateEstimate(kDefaultSsrc, time_now);
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_FALSE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  
  bitrate_estimator_->IncomingPacket(kDefaultSsrc, kMtu, time_now,
                                     timestamp);
  bitrate_estimator_->UpdateEstimate(kDefaultSsrc, time_now);
  EXPECT_FALSE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_EQ(0u, ssrcs.size());
  EXPECT_FALSE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  
  
  
  time_now += 499;
  bitrate_estimator_->IncomingPacket(kDefaultSsrc, kMtu, time_now,
                                     timestamp);
  time_now += 2;
  bitrate_estimator_->UpdateEstimate(kDefaultSsrc, time_now);
  EXPECT_TRUE(bitrate_estimator_->LatestEstimate(&ssrcs, &bitrate_bps));
  EXPECT_EQ(1u, ssrcs.size());
  EXPECT_EQ(kDefaultSsrc, ssrcs.front());
  EXPECT_EQ(20607u, bitrate_bps);
  EXPECT_TRUE(bitrate_observer_->updated());
  bitrate_observer_->Reset();
  EXPECT_EQ(bitrate_observer_->latest_bitrate(), bitrate_bps);
}

TEST_F(RemoteBitrateEstimatorTest, TestRateIncreaseReordering) {
  int64_t time_now = 0;
  uint32_t timestamp = 0;
  const int framerate = 50;  
  const int frame_interval_ms = 1000 / framerate;
  bitrate_estimator_->IncomingPacket(kDefaultSsrc, 1000, time_now, timestamp);
  bitrate_estimator_->UpdateEstimate(kDefaultSsrc, time_now);
  EXPECT_FALSE(bitrate_observer_->updated());  
  
  time_now += 1000;
  timestamp += 90 * 1000;
  bitrate_estimator_->IncomingPacket(kDefaultSsrc, 1000, time_now, timestamp);
  bitrate_estimator_->UpdateEstimate(kDefaultSsrc, time_now);
  EXPECT_TRUE(bitrate_observer_->updated());
  EXPECT_EQ(17645u, bitrate_observer_->latest_bitrate());
  for (int i = 0; i < 10; ++i) {
    time_now += 2 * frame_interval_ms;
    timestamp += 2 * 90 * frame_interval_ms;
    bitrate_estimator_->IncomingPacket(kDefaultSsrc, 1000, time_now, timestamp);
    bitrate_estimator_->IncomingPacket(kDefaultSsrc,
                                       1000,
                                       time_now - frame_interval_ms,
                                       timestamp - 90 * frame_interval_ms);
  }
  bitrate_estimator_->UpdateEstimate(kDefaultSsrc, time_now);
  EXPECT_TRUE(bitrate_observer_->updated());
  EXPECT_EQ(18985u, bitrate_observer_->latest_bitrate());
}


TEST_F(RemoteBitrateEstimatorTest, TestRateIncreaseRtpTimestamps) {
  const int kExpectedIterations = 276;
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
  const int kNumberOfFrames= 300;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  AddDefaultStream();
  
  stream_generator_->set_capacity_bps(1000e3);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc, kNumberOfFrames,
                                            kStartBitrate, kMinExpectedBitrate,
                                            kMaxExpectedBitrate);
  
  stream_generator_->set_capacity_bps(500e3);
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= 500e3) {
        bitrate_drop_time = time_now_;
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(10333, bitrate_drop_time);
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
  
  stream_generator_->set_capacity_bps(1000e3);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            kSteadyStateTime * kFramerate,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate);
  bitrate_observer_->Reset();
  
  stream_generator_->set_capacity_bps(500e3);
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= 500e3) {
        bitrate_drop_time = time_now_;
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(8299, bitrate_drop_time);
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
  
  stream_generator_->set_capacity_bps(1000e3);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            kSteadyStateTime * kFramerate,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate);
  bitrate_observer_->Reset();
  
  stream_generator_->set_capacity_bps(500e3);
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= 500e3) {
        bitrate_drop_time = time_now_;
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(8299, bitrate_drop_time);
}




TEST_F(RemoteBitrateEstimatorTestAlign, TwoStreamsCapacityDropWithWrap) {
  const int kFramerate= 30;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  const int kSteadyStateTime = 7;  
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
      std::numeric_limits<uint32_t>::max() - kSteadyStateTime * 90000);
  
  stream_generator_->set_capacity_bps(1000e3);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            kSteadyStateTime * kFramerate,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate);
  bitrate_observer_->Reset();
  
  stream_generator_->set_capacity_bps(500e3);
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= 500e3) {
        bitrate_drop_time = time_now_;
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(4933, bitrate_drop_time);
}




TEST_F(RemoteBitrateEstimatorTestAlign, ThreeStreams) {
  const int kFramerate= 30;
  const int kStartBitrate = 900e3;
  const int kMinExpectedBitrate = 800e3;
  const int kMaxExpectedBitrate = 1100e3;
  const int kSteadyStateTime = 11;  
  stream_generator_->AddStream(new testing::RtpStream(
      30,           
      kStartBitrate/2,  
      1,            
      90000,        
      0xFFFFF000,   
      0));          

  stream_generator_->AddStream(new testing::RtpStream(
      30,           
      kStartBitrate/3,  
      2,            
      90000,        
      0x00000FFF,   
      0));          

  stream_generator_->AddStream(new testing::RtpStream(
      30,           
      kStartBitrate/6,  
      3,            
      90000,        
      0x00000FFF,   
      0));          
  
  stream_generator_->set_rtp_timestamp_offset(kDefaultSsrc,
      std::numeric_limits<uint32_t>::max() - kSteadyStateTime * 90000);
  
  stream_generator_->set_capacity_bps(1000e3);
  unsigned int bitrate_bps = SteadyStateRun(kDefaultSsrc,
                                            kSteadyStateTime * kFramerate,
                                            kStartBitrate,
                                            kMinExpectedBitrate,
                                            kMaxExpectedBitrate);
  bitrate_observer_->Reset();
  
  stream_generator_->set_capacity_bps(500e3);
  int64_t bitrate_drop_time = -1;
  for (int i = 0; i < 200; ++i) {
    GenerateAndProcessFrame(kDefaultSsrc, bitrate_bps);
    
    if (bitrate_observer_->updated()) {
      if (bitrate_drop_time == -1 &&
          bitrate_observer_->latest_bitrate() <= 500e3) {
        bitrate_drop_time = time_now_;
      }
      bitrate_bps = bitrate_observer_->latest_bitrate();
      bitrate_observer_->Reset();
    }
  }
  EXPECT_EQ(3966, bitrate_drop_time);
}

}  
