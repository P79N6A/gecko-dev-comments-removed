











#include "webrtc/modules/audio_coding/neteq4/buffer_level_filter.h"

#include <math.h>  

#include "gtest/gtest.h"

namespace webrtc {

TEST(BufferLevelFilter, CreateAndDestroy) {
  BufferLevelFilter* filter = new BufferLevelFilter();
  EXPECT_EQ(0, filter->filtered_current_level());
  delete filter;
}

TEST(BufferLevelFilter, ConvergenceTest) {
  BufferLevelFilter filter;
  for (int times = 10; times <= 50; times += 10) {
    for (int value = 100; value <= 200; value += 10) {
      filter.Reset();
      filter.SetTargetBufferLevel(1);  
      std::ostringstream ss;
      ss << "times = " << times << ", value = " << value;
      SCOPED_TRACE(ss.str());  
      for (int i = 0; i < times; ++i) {
        filter.Update(value, 0 ,
                      160 );
      }
      
      
      double expected_value_double =
          (1 - pow(251.0 / 256.0, times)) * value;
      int expected_value = static_cast<int>(expected_value_double);
      
      
      
      
      EXPECT_NEAR(expected_value, filter.filtered_current_level() >> 8, 1);
    }
  }
}


TEST(BufferLevelFilter, FilterFactor) {
  BufferLevelFilter filter;
  
  const int kTimes = 10;
  const int kValue = 100;

  filter.SetTargetBufferLevel(3);  
  for (int i = 0; i < kTimes; ++i) {
    filter.Update(kValue, 0 ,
                  160 );
  }
  
  
  int expected_value = 14;
  
  EXPECT_EQ(expected_value, filter.filtered_current_level() >> 8);

  filter.Reset();
  filter.SetTargetBufferLevel(7);  
  for (int i = 0; i < kTimes; ++i) {
    filter.Update(kValue, 0 ,
                  160 );
  }
  
  
  expected_value = 11;
  
  EXPECT_EQ(expected_value, filter.filtered_current_level() >> 8);

  filter.Reset();
  filter.SetTargetBufferLevel(8);  
  for (int i = 0; i < kTimes; ++i) {
    filter.Update(kValue, 0 ,
                  160 );
  }
  
  
  expected_value = 7;
  
  EXPECT_EQ(expected_value, filter.filtered_current_level() >> 8);
}


TEST(BufferLevelFilter, TimeStretchedSamples) {
  BufferLevelFilter filter;
  filter.SetTargetBufferLevel(1);  
  
  const int kTimes = 10;
  const int kValue = 100;
  const int kPacketSizeSamples = 160;
  const int kNumPacketsStretched = 2;
  const int kTimeStretchedSamples = kNumPacketsStretched * kPacketSizeSamples;
  for (int i = 0; i < kTimes; ++i) {
    
    
    filter.Update(kValue, kTimeStretchedSamples, 0 );
  }
  
  
  const int kExpectedValue = 17;
  
  EXPECT_EQ(kExpectedValue, filter.filtered_current_level() >> 8);

  
  
  
  filter.Update(filter.filtered_current_level() >> 8, kTimeStretchedSamples,
                kPacketSizeSamples);
  EXPECT_EQ(kExpectedValue - kNumPacketsStretched,
            filter.filtered_current_level() >> 8);
  
  filter.Update(filter.filtered_current_level() >> 8, -kTimeStretchedSamples,
                kPacketSizeSamples);
  EXPECT_EQ(kExpectedValue, filter.filtered_current_level() >> 8);
}

TEST(BufferLevelFilter, TimeStretchedSamplesNegativeUnevenFrames) {
  BufferLevelFilter filter;
  filter.SetTargetBufferLevel(1);  
  
  const int kTimes = 10;
  const int kValue = 100;
  const int kPacketSizeSamples = 160;
  const int kTimeStretchedSamples = -3.1415 * kPacketSizeSamples;
  for (int i = 0; i < kTimes; ++i) {
    
    
    filter.Update(kValue, kTimeStretchedSamples, 0 );
  }
  
  
  const int kExpectedValue = 17;
  
  EXPECT_EQ(kExpectedValue, filter.filtered_current_level() >> 8);

  
  
  
  filter.Update(filter.filtered_current_level() >> 8, kTimeStretchedSamples,
                kPacketSizeSamples);
  EXPECT_EQ(21, filter.filtered_current_level() >> 8);
  
  filter.Update(filter.filtered_current_level() >> 8, -kTimeStretchedSamples,
                kPacketSizeSamples);
  EXPECT_EQ(kExpectedValue, filter.filtered_current_level() >> 8);
}

}  
