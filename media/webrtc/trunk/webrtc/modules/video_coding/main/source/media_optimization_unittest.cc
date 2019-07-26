









#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/video_coding/main/source/media_optimization.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {
namespace media_optimization {

class TestMediaOptimization : public ::testing::Test {
 protected:
  enum {
    kId = 4711  
  };
  enum {
    kSampleRate = 90000  
  };

  
  
  TestMediaOptimization()
      : clock_(1000),
        media_opt_(kId, &clock_),
        frame_time_ms_(33),
        next_timestamp_(0) {}

  
  void AddFrameAndAdvanceTime(int bitrate_bps, bool expect_frame_drop) {
    ASSERT_GE(bitrate_bps, 0);
    bool frame_dropped = media_opt_.DropFrame();
    EXPECT_EQ(expect_frame_drop, frame_dropped);
    if (!frame_dropped) {
      int bytes_per_frame = bitrate_bps * frame_time_ms_ / (8 * 1000);
      ASSERT_EQ(VCM_OK, media_opt_.UpdateWithEncodedData(
          bytes_per_frame, next_timestamp_, kVideoFrameDelta));
    }
    next_timestamp_ += frame_time_ms_ * kSampleRate / 1000;
    clock_.AdvanceTimeMilliseconds(frame_time_ms_);
  }

  SimulatedClock clock_;
  MediaOptimization media_opt_;
  int frame_time_ms_;
  uint32_t next_timestamp_;
};


TEST_F(TestMediaOptimization, VerifyMuting) {
  
  
  
  const int kThresholdBps = 50000;
  const int kWindowBps = 10000;
  media_opt_.SuspendBelowMinBitrate(kThresholdBps, kWindowBps);

  
  EXPECT_FALSE(media_opt_.IsVideoSuspended());

  int target_bitrate_kbps = 100;
  media_opt_.SetTargetRates(target_bitrate_kbps * 1000,
                            0,  
                            100,
                            NULL,
                            NULL);  
  media_opt_.EnableFrameDropper(true);
  for (int time = 0; time < 2000; time += frame_time_ms_) {
    ASSERT_NO_FATAL_FAILURE(AddFrameAndAdvanceTime(target_bitrate_kbps, false));
  }

  
  media_opt_.SetTargetRates(kThresholdBps - 1000,
                            0,  
                            100,
                            NULL,
                            NULL);  
  
  
  for (int time = 0; time < 2000; time += frame_time_ms_) {
    EXPECT_TRUE(media_opt_.IsVideoSuspended());
    ASSERT_NO_FATAL_FAILURE(AddFrameAndAdvanceTime(target_bitrate_kbps, true));
  }

  
  
  media_opt_.SetTargetRates(kThresholdBps + 1000,
                            0,  
                            100,
                            NULL,
                            NULL);  
                                    
  
  for (int time = 0; time < 2000; time += frame_time_ms_) {
    EXPECT_TRUE(media_opt_.IsVideoSuspended());
    ASSERT_NO_FATAL_FAILURE(AddFrameAndAdvanceTime(target_bitrate_kbps, true));
  }

  
  media_opt_.SetTargetRates(kThresholdBps + kWindowBps + 1000,
                            0,  
                            100,
                            NULL,
                            NULL);  
  
  
  for (int time = 0; time < 2000; time += frame_time_ms_) {
    EXPECT_FALSE(media_opt_.IsVideoSuspended());
    ASSERT_NO_FATAL_FAILURE(
        AddFrameAndAdvanceTime((kThresholdBps + kWindowBps) / 1000, false));
  }
}

}  
}  
