











#include "webrtc/modules/audio_coding/neteq4/delay_peak_detector.h"

#include "gtest/gtest.h"

namespace webrtc {

TEST(DelayPeakDetector, CreateAndDestroy) {
  DelayPeakDetector* detector = new DelayPeakDetector();
  EXPECT_FALSE(detector->peak_found());
  delete detector;
}

TEST(DelayPeakDetector, EmptyHistory) {
  DelayPeakDetector detector;
  EXPECT_EQ(-1, detector.MaxPeakHeight());
  EXPECT_EQ(-1, detector.MaxPeakPeriod());
}




TEST(DelayPeakDetector, TriggerPeakMode) {
  DelayPeakDetector detector;
  const int kPacketSizeMs = 30;
  detector.SetPacketAudioLength(kPacketSizeMs);

  
  const int kNumPackets = 1000;
  int arrival_times_ms[kNumPackets];
  for (int i = 0; i < kNumPackets; ++i) {
    arrival_times_ms[i] = i * kPacketSizeMs;
  }

  
  const int kPeakDelayMs = 100;
  
  arrival_times_ms[100] += kPeakDelayMs;
  
  arrival_times_ms[200] += kPeakDelayMs;
  
  arrival_times_ms[400] += kPeakDelayMs;
  
  const int kWorstPeakPeriod = 200 * kPacketSizeMs;
  int peak_mode_start_ms = arrival_times_ms[400];
  
  int peak_mode_end_ms = peak_mode_start_ms + 2 * kWorstPeakPeriod;

  
  int time = 0;
  int next = 1;  
  while (next < kNumPackets) {
    while (next < kNumPackets && arrival_times_ms[next] <= time) {
      int iat_packets = (arrival_times_ms[next] - arrival_times_ms[next - 1]) /
          kPacketSizeMs;
      const int kTargetBufferLevel = 1;  
      if (time < peak_mode_start_ms || time > peak_mode_end_ms) {
        EXPECT_FALSE(detector.Update(iat_packets, kTargetBufferLevel));
      } else {
        EXPECT_TRUE(detector.Update(iat_packets, kTargetBufferLevel));
        EXPECT_EQ(kWorstPeakPeriod, detector.MaxPeakPeriod());
        EXPECT_EQ(kPeakDelayMs / kPacketSizeMs + 1, detector.MaxPeakHeight());
      }
      ++next;
    }
    detector.IncrementCounter(10);
    time += 10;  
  }
}




TEST(DelayPeakDetector, DoNotTriggerPeakMode) {
  DelayPeakDetector detector;
  const int kPacketSizeMs = 30;
  detector.SetPacketAudioLength(kPacketSizeMs);

  
  const int kNumPackets = 1000;
  int arrival_times_ms[kNumPackets];
  for (int i = 0; i < kNumPackets; ++i) {
    arrival_times_ms[i] = i * kPacketSizeMs;
  }

  
  const int kPeakDelayMs = 100;
  
  arrival_times_ms[100] += kPeakDelayMs;
  
  arrival_times_ms[200] += kPeakDelayMs;
  
  arrival_times_ms[400] += kPeakDelayMs;

  
  int time = 0;
  int next = 1;  
  while (next < kNumPackets) {
    while (next < kNumPackets && arrival_times_ms[next] <= time) {
      int iat_packets = (arrival_times_ms[next] - arrival_times_ms[next - 1]) /
          kPacketSizeMs;
      const int kTargetBufferLevel = 2;  
      EXPECT_FALSE(detector.Update(iat_packets, kTargetBufferLevel));
      ++next;
    }
    detector.IncrementCounter(10);
    time += 10;  
  }
}
}  
