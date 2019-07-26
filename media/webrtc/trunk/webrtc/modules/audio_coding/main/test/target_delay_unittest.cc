









#include "gtest/gtest.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace webrtc {
class TargetDelayTest : public ::testing::Test {
 protected:
  static const int kSampleRateHz = 16000;
  static const int kNum10msPerFrame = 2;
  static const int kFrameSizeSamples = 320;  
  
  static const int kPayloadLenBytes = 320 * 2;
  
  
  static const int kInterarrivalJitterPacket = 2;

  TargetDelayTest()
      : acm_(AudioCodingModule::Create(0)) {}

  ~TargetDelayTest() {
  }

  void SetUp() {
    EXPECT_TRUE(acm_.get() != NULL);

    CodecInst codec;
    ASSERT_EQ(0, AudioCodingModule::Codec("L16", &codec, kSampleRateHz, 1));
    ASSERT_EQ(0, acm_->InitializeReceiver());
    ASSERT_EQ(0, acm_->RegisterReceiveCodec(codec));

    rtp_info_.header.payloadType = codec.pltype;
    rtp_info_.header.timestamp = 0;
    rtp_info_.header.ssrc = 0x12345678;
    rtp_info_.header.markerBit = false;
    rtp_info_.header.sequenceNumber = 0;
    rtp_info_.type.Audio.channel = 1;
    rtp_info_.type.Audio.isCNG = false;
    rtp_info_.frameType = kAudioFrameSpeech;
  }

  void Push() {
    rtp_info_.header.timestamp += kFrameSizeSamples;
    rtp_info_.header.sequenceNumber++;
    uint8_t payload[kPayloadLenBytes];  
    ASSERT_EQ(0, acm_->IncomingPacket(payload, kFrameSizeSamples * 2,
                                      rtp_info_));
  }

  
  void Pull() {
    AudioFrame frame;
    for (int k = 0; k < kNum10msPerFrame; ++k) {  
      ASSERT_EQ(0, acm_->PlayoutData10Ms(-1, &frame));
      
      ASSERT_TRUE(kSampleRateHz == frame.sample_rate_hz_);
      ASSERT_EQ(1, frame.num_channels_);
      ASSERT_TRUE(kSampleRateHz / 100 == frame.samples_per_channel_);
    }
  }

  void Run(bool clean) {
    for (int n = 0; n < 10; ++n) {
      for (int m = 0; m < 5; ++m) {
        Push();
        Pull();
      }

      if (!clean) {
        for (int m = 0; m < 10; ++m) {  
          Push();
          for (int n = 0; n < kInterarrivalJitterPacket; ++n)
            Pull();
        }
      }
    }
  }

  int SetMinimumDelay(int delay_ms) {
    return acm_->SetMinimumPlayoutDelay(delay_ms);
  }

  int SetMaximumDelay(int delay_ms) {
    return acm_->SetMaximumPlayoutDelay(delay_ms);
  }

  int GetCurrentOptimalDelayMs() {
    ACMNetworkStatistics stats;
    acm_->NetworkStatistics(&stats);
    return stats.preferredBufferSize;
  }

  int RequiredDelay() {
    return acm_->LeastRequiredDelayMs();
  }

  scoped_ptr<AudioCodingModule> acm_;
  WebRtcRTPHeader rtp_info_;
};

TEST_F(TargetDelayTest, DISABLED_ON_ANDROID(OutOfRangeInput)) {
  EXPECT_EQ(-1, SetMinimumDelay(-1));
  EXPECT_EQ(-1, SetMinimumDelay(10001));
}

TEST_F(TargetDelayTest, DISABLED_ON_ANDROID(NoTargetDelayBufferSizeChanges)) {
  for (int n = 0; n < 30; ++n)  
    Run(true);
  int clean_optimal_delay = GetCurrentOptimalDelayMs();
  Run(false);  
  int jittery_optimal_delay = GetCurrentOptimalDelayMs();
  EXPECT_GT(jittery_optimal_delay, clean_optimal_delay);
  int required_delay = RequiredDelay();
  EXPECT_GT(required_delay, 0);
  EXPECT_NEAR(required_delay, jittery_optimal_delay, 1);
}

TEST_F(TargetDelayTest, DISABLED_ON_ANDROID(WithTargetDelayBufferNotChanging)) {
  
  const int kTargetDelayMs = (kInterarrivalJitterPacket + 1) *
      kNum10msPerFrame * 10;
  ASSERT_EQ(0, SetMinimumDelay(kTargetDelayMs));
  for (int n = 0; n < 30; ++n)  
    Run(true);
  int clean_optimal_delay = GetCurrentOptimalDelayMs();
  EXPECT_EQ(kTargetDelayMs, clean_optimal_delay);
  Run(false);  
  int jittery_optimal_delay = GetCurrentOptimalDelayMs();
  EXPECT_EQ(jittery_optimal_delay, clean_optimal_delay);
}

TEST_F(TargetDelayTest, DISABLED_ON_ANDROID(RequiredDelayAtCorrectRange)) {
  for (int n = 0; n < 30; ++n)  
    Run(true);
  int clean_optimal_delay = GetCurrentOptimalDelayMs();

  
  const int kTargetDelayMs = (kInterarrivalJitterPacket + 10) *
      kNum10msPerFrame * 10;
  ASSERT_EQ(0, SetMinimumDelay(kTargetDelayMs));
  for (int n = 0; n < 300; ++n)  
    Run(true);
  Run(false);  

  int jittery_optimal_delay = GetCurrentOptimalDelayMs();
  EXPECT_EQ(kTargetDelayMs, jittery_optimal_delay);

  int required_delay = RequiredDelay();

  
  EXPECT_GT(required_delay, 0);
  EXPECT_GT(jittery_optimal_delay, required_delay);
  EXPECT_GT(required_delay, clean_optimal_delay);

  
  
  
  
  EXPECT_NEAR(kInterarrivalJitterPacket * kNum10msPerFrame * 10,
              required_delay, 1);
}

TEST_F(TargetDelayTest, DISABLED_ON_ANDROID(TargetDelayBufferMinMax)) {
  const int kTargetMinDelayMs = kNum10msPerFrame * 10;
  ASSERT_EQ(0, SetMinimumDelay(kTargetMinDelayMs));
  for (int m = 0; m < 30; ++m)  
    Run(true);
  int clean_optimal_delay = GetCurrentOptimalDelayMs();
  EXPECT_EQ(kTargetMinDelayMs, clean_optimal_delay);

  const int kTargetMaxDelayMs = 2 * (kNum10msPerFrame * 10);
  ASSERT_EQ(0, SetMaximumDelay(kTargetMaxDelayMs));
  for (int n = 0; n < 30; ++n)  
    Run(false);

  int capped_optimal_delay = GetCurrentOptimalDelayMs();
  EXPECT_EQ(kTargetMaxDelayMs, capped_optimal_delay);
}

}  
