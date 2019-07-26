









#include <math.h>
#include <algorithm>

#include "gtest/gtest.h"
#include "video_engine/stream_synchronization.h"

namespace webrtc {


enum { kMaxVideoDiffMs = 80 };
enum { kMaxAudioDiffMs = 80 };
enum { kMaxDelay = 1500 };


enum { kDefaultAudioFrequency = 8000 };
enum { kDefaultVideoFrequency = 90000 };
const double kNtpFracPerMs = 4.294967296E6;
static const int kSmoothingFilter = 4 * 2;

class Time {
 public:
  explicit Time(int64_t offset)
      : kNtpJan1970(2208988800UL),
        time_now_ms_(offset) {}

    synchronization::RtcpMeasurement GenerateRtcp(int frequency,
                                                  uint32_t offset) const {
    synchronization::RtcpMeasurement rtcp;
    NowNtp(&rtcp.ntp_secs, &rtcp.ntp_frac);
    rtcp.rtp_timestamp = NowRtp(frequency, offset);
    return rtcp;
  }

  void NowNtp(uint32_t* ntp_secs, uint32_t* ntp_frac) const {
    *ntp_secs = time_now_ms_ / 1000 + kNtpJan1970;
    int64_t remainder_ms = time_now_ms_ % 1000;
    *ntp_frac = static_cast<uint32_t>(
        static_cast<double>(remainder_ms) * kNtpFracPerMs + 0.5);
  }

  uint32_t NowRtp(int frequency, uint32_t offset) const {
    return frequency * time_now_ms_ / 1000 + offset;
  }

  void IncreaseTimeMs(int64_t inc) {
    time_now_ms_ += inc;
  }

  int64_t time_now_ms() const {
    return time_now_ms_;
  }

 private:
  
  const uint32_t kNtpJan1970;
  int64_t time_now_ms_;
};

class StreamSynchronizationTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    sync_ = new StreamSynchronization(0, 0);
    send_time_ = new Time(kSendTimeOffsetMs);
    receive_time_ = new Time(kReceiveTimeOffsetMs);
    audio_clock_drift_ = 1.0;
    video_clock_drift_ = 1.0;
  }

  virtual void TearDown() {
    delete sync_;
    delete send_time_;
    delete receive_time_;
  }

  
  
  
  
  
  
  bool DelayedStreams(int audio_delay_ms,
                      int video_delay_ms,
                      int current_audio_delay_ms,
                      int* extra_audio_delay_ms,
                      int* total_video_delay_ms) {
    int audio_frequency = static_cast<int>(kDefaultAudioFrequency *
                                           audio_clock_drift_ + 0.5);
    int audio_offset = 0;
    int video_frequency = static_cast<int>(kDefaultVideoFrequency *
                                           video_clock_drift_ + 0.5);
    int video_offset = 0;
    StreamSynchronization::Measurements audio;
    StreamSynchronization::Measurements video;
    
    audio.rtcp.push_front(send_time_->GenerateRtcp(audio_frequency,
                                                   audio_offset));
    send_time_->IncreaseTimeMs(100);
    receive_time_->IncreaseTimeMs(100);
    video.rtcp.push_front(send_time_->GenerateRtcp(video_frequency,
                                                   video_offset));
    send_time_->IncreaseTimeMs(900);
    receive_time_->IncreaseTimeMs(900);
    audio.rtcp.push_front(send_time_->GenerateRtcp(audio_frequency,
                                                   audio_offset));
    send_time_->IncreaseTimeMs(100);
    receive_time_->IncreaseTimeMs(100);
    video.rtcp.push_front(send_time_->GenerateRtcp(video_frequency,
                                                   video_offset));
    send_time_->IncreaseTimeMs(900);
    receive_time_->IncreaseTimeMs(900);

    
    audio.latest_timestamp = send_time_->NowRtp(audio_frequency,
                                                audio_offset);
    video.latest_timestamp = send_time_->NowRtp(video_frequency,
                                                video_offset);

    if (audio_delay_ms > video_delay_ms) {
      
      receive_time_->IncreaseTimeMs(video_delay_ms);
      video.latest_receive_time_ms = receive_time_->time_now_ms();
      receive_time_->IncreaseTimeMs(audio_delay_ms - video_delay_ms);
      audio.latest_receive_time_ms = receive_time_->time_now_ms();
    } else {
      
      receive_time_->IncreaseTimeMs(audio_delay_ms);
      audio.latest_receive_time_ms = receive_time_->time_now_ms();
      receive_time_->IncreaseTimeMs(video_delay_ms - audio_delay_ms);
      video.latest_receive_time_ms = receive_time_->time_now_ms();
    }
    int relative_delay_ms;
    StreamSynchronization::ComputeRelativeDelay(audio, video,
                                                &relative_delay_ms);
    EXPECT_EQ(video_delay_ms - audio_delay_ms, relative_delay_ms);
    return sync_->ComputeDelays(relative_delay_ms,
                                current_audio_delay_ms,
                                extra_audio_delay_ms,
                                total_video_delay_ms);
  }

  
  
  
  
  
  
  
  void BothDelayedAudioLaterTest(int base_target_delay) {
    int current_audio_delay_ms = base_target_delay;
    int audio_delay_ms = base_target_delay + 300;
    int video_delay_ms = base_target_delay + 100;
    int extra_audio_delay_ms = 0;
    int total_video_delay_ms = base_target_delay;
    int filtered_move = (audio_delay_ms - video_delay_ms) / kSmoothingFilter;
    const int kNeteqDelayIncrease = 50;
    const int kNeteqDelayDecrease = 10;

    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    EXPECT_EQ(base_target_delay + filtered_move, total_video_delay_ms);
    EXPECT_EQ(base_target_delay, extra_audio_delay_ms);
    current_audio_delay_ms = extra_audio_delay_ms;

    send_time_->IncreaseTimeMs(1000);
    receive_time_->IncreaseTimeMs(1000 - std::max(audio_delay_ms,
                                                  video_delay_ms));
    
    total_video_delay_ms = base_target_delay;
    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    EXPECT_EQ(base_target_delay + 2 * filtered_move, total_video_delay_ms);
    EXPECT_EQ(base_target_delay, extra_audio_delay_ms);
    current_audio_delay_ms = extra_audio_delay_ms;

    send_time_->IncreaseTimeMs(1000);
    receive_time_->IncreaseTimeMs(1000 - std::max(audio_delay_ms,
                                                  video_delay_ms));
    
    total_video_delay_ms = base_target_delay;
    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    EXPECT_EQ(base_target_delay + 3 * filtered_move, total_video_delay_ms);
    EXPECT_EQ(base_target_delay, extra_audio_delay_ms);

    
    current_audio_delay_ms = base_target_delay + kNeteqDelayIncrease;
    send_time_->IncreaseTimeMs(1000);
    receive_time_->IncreaseTimeMs(1000 - std::max(audio_delay_ms,
                                                  video_delay_ms));
    
    total_video_delay_ms = base_target_delay;
    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    filtered_move = 3 * filtered_move +
        (kNeteqDelayIncrease + audio_delay_ms - video_delay_ms) /
        kSmoothingFilter;
    EXPECT_EQ(base_target_delay + filtered_move, total_video_delay_ms);
    EXPECT_EQ(base_target_delay, extra_audio_delay_ms);

    
    current_audio_delay_ms = base_target_delay + kNeteqDelayDecrease;
    send_time_->IncreaseTimeMs(1000);
    receive_time_->IncreaseTimeMs(1000 - std::max(audio_delay_ms,
                                                  video_delay_ms));
    
    total_video_delay_ms = base_target_delay;
    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));

    filtered_move = filtered_move +
        (kNeteqDelayDecrease + audio_delay_ms - video_delay_ms) /
        kSmoothingFilter;

    EXPECT_EQ(base_target_delay + filtered_move, total_video_delay_ms);
    EXPECT_EQ(base_target_delay, extra_audio_delay_ms);
  }

  void BothDelayedVideoLaterTest(int base_target_delay) {
    int current_audio_delay_ms = base_target_delay;
    int audio_delay_ms = base_target_delay + 100;
    int video_delay_ms = base_target_delay + 300;
    int extra_audio_delay_ms = 0;
    int total_video_delay_ms = base_target_delay;

    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    EXPECT_EQ(base_target_delay, total_video_delay_ms);
    
    EXPECT_GE(base_target_delay + kMaxAudioDiffMs, extra_audio_delay_ms);
    current_audio_delay_ms = extra_audio_delay_ms;
    int current_extra_delay_ms = extra_audio_delay_ms;

    send_time_->IncreaseTimeMs(1000);
    receive_time_->IncreaseTimeMs(800);
    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    EXPECT_EQ(base_target_delay, total_video_delay_ms);
    
    
    EXPECT_EQ(current_extra_delay_ms + MaxAudioDelayIncrease(
        current_audio_delay_ms,
        base_target_delay + video_delay_ms - audio_delay_ms),
        extra_audio_delay_ms);
    current_audio_delay_ms = extra_audio_delay_ms;
    current_extra_delay_ms = extra_audio_delay_ms;

    send_time_->IncreaseTimeMs(1000);
    receive_time_->IncreaseTimeMs(800);
    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    EXPECT_EQ(base_target_delay, total_video_delay_ms);
    
    
    EXPECT_EQ(current_extra_delay_ms + MaxAudioDelayIncrease(
        current_audio_delay_ms,
        base_target_delay + video_delay_ms - audio_delay_ms),
        extra_audio_delay_ms);
    current_extra_delay_ms = extra_audio_delay_ms;

    
    current_audio_delay_ms = base_target_delay + 10;
    send_time_->IncreaseTimeMs(1000);
    receive_time_->IncreaseTimeMs(800);
    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    EXPECT_EQ(base_target_delay, total_video_delay_ms);
    
    
    
    EXPECT_EQ(current_extra_delay_ms + MaxAudioDelayIncrease(
        current_audio_delay_ms,
        base_target_delay + video_delay_ms - audio_delay_ms),
        extra_audio_delay_ms);
    current_extra_delay_ms = extra_audio_delay_ms;

    
    current_audio_delay_ms = base_target_delay + 350;
    send_time_->IncreaseTimeMs(1000);
    receive_time_->IncreaseTimeMs(800);
    EXPECT_TRUE(DelayedStreams(audio_delay_ms,
                               video_delay_ms,
                               current_audio_delay_ms,
                               &extra_audio_delay_ms,
                               &total_video_delay_ms));
    EXPECT_EQ(base_target_delay, total_video_delay_ms);
    
    
    EXPECT_EQ(current_extra_delay_ms + MaxAudioDelayIncrease(
        current_audio_delay_ms,
        base_target_delay + video_delay_ms - audio_delay_ms),
        extra_audio_delay_ms);
  }

  int MaxAudioDelayIncrease(int current_audio_delay_ms, int delay_ms) {
    return std::min((delay_ms - current_audio_delay_ms) / kSmoothingFilter,
                     static_cast<int>(kMaxAudioDiffMs));
  }

  int MaxAudioDelayDecrease(int current_audio_delay_ms, int delay_ms) {
    return std::max((delay_ms - current_audio_delay_ms) / kSmoothingFilter,
                    -kMaxAudioDiffMs);
  }

  enum { kSendTimeOffsetMs = 98765 };
  enum { kReceiveTimeOffsetMs = 43210 };

  StreamSynchronization* sync_;
  Time* send_time_;  
  Time* receive_time_;  
  double audio_clock_drift_;
  double video_clock_drift_;
};

TEST_F(StreamSynchronizationTest, NoDelay) {
  uint32_t current_audio_delay_ms = 0;
  int extra_audio_delay_ms = 0;
  int total_video_delay_ms = 0;

  EXPECT_FALSE(DelayedStreams(0, 0, current_audio_delay_ms,
                              &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, extra_audio_delay_ms);
  EXPECT_EQ(0, total_video_delay_ms);
}

TEST_F(StreamSynchronizationTest, VideoDelay) {
  uint32_t current_audio_delay_ms = 0;
  int delay_ms = 200;
  int extra_audio_delay_ms = 0;
  int total_video_delay_ms = 0;

  EXPECT_TRUE(DelayedStreams(delay_ms, 0, current_audio_delay_ms,
                             &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, extra_audio_delay_ms);
  
  EXPECT_EQ(delay_ms / kSmoothingFilter, total_video_delay_ms);

  send_time_->IncreaseTimeMs(1000);
  receive_time_->IncreaseTimeMs(800);
  
  total_video_delay_ms = 0;
  EXPECT_TRUE(DelayedStreams(delay_ms, 0, current_audio_delay_ms,
                             &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, extra_audio_delay_ms);
  
  EXPECT_EQ(2 * delay_ms / kSmoothingFilter, total_video_delay_ms);

  send_time_->IncreaseTimeMs(1000);
  receive_time_->IncreaseTimeMs(800);
  
  total_video_delay_ms = 0;
  EXPECT_TRUE(DelayedStreams(delay_ms, 0, current_audio_delay_ms,
                             &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, extra_audio_delay_ms);
  EXPECT_EQ(3 * delay_ms / kSmoothingFilter, total_video_delay_ms);
}

TEST_F(StreamSynchronizationTest, AudioDelay) {
  int current_audio_delay_ms = 0;
  int delay_ms = 200;
  int extra_audio_delay_ms = 0;
  int total_video_delay_ms = 0;

  EXPECT_TRUE(DelayedStreams(0, delay_ms, current_audio_delay_ms,
                             &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, total_video_delay_ms);
  
  EXPECT_EQ(delay_ms / kSmoothingFilter, extra_audio_delay_ms);
  current_audio_delay_ms = extra_audio_delay_ms;
  int current_extra_delay_ms = extra_audio_delay_ms;

  send_time_->IncreaseTimeMs(1000);
  receive_time_->IncreaseTimeMs(800);
  EXPECT_TRUE(DelayedStreams(0, delay_ms, current_audio_delay_ms,
                             &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, total_video_delay_ms);
  
  
  EXPECT_EQ(current_extra_delay_ms +
            MaxAudioDelayIncrease(current_audio_delay_ms, delay_ms),
            extra_audio_delay_ms);
  current_audio_delay_ms = extra_audio_delay_ms;
  current_extra_delay_ms = extra_audio_delay_ms;

  send_time_->IncreaseTimeMs(1000);
  receive_time_->IncreaseTimeMs(800);
  EXPECT_TRUE(DelayedStreams(0, delay_ms, current_audio_delay_ms,
                             &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, total_video_delay_ms);
  
  
  EXPECT_EQ(current_extra_delay_ms +
            MaxAudioDelayIncrease(current_audio_delay_ms, delay_ms),
            extra_audio_delay_ms);
  current_extra_delay_ms = extra_audio_delay_ms;

  
  current_audio_delay_ms = 10;
  send_time_->IncreaseTimeMs(1000);
  receive_time_->IncreaseTimeMs(800);
  EXPECT_TRUE(DelayedStreams(0, delay_ms, current_audio_delay_ms,
                             &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, total_video_delay_ms);
  
  
  
  EXPECT_EQ(current_extra_delay_ms +
            MaxAudioDelayIncrease(current_audio_delay_ms, delay_ms),
            extra_audio_delay_ms);
  current_extra_delay_ms = extra_audio_delay_ms;

  
  current_audio_delay_ms = 350;
  send_time_->IncreaseTimeMs(1000);
  receive_time_->IncreaseTimeMs(800);
  EXPECT_TRUE(DelayedStreams(0, delay_ms, current_audio_delay_ms,
                             &extra_audio_delay_ms, &total_video_delay_ms));
  EXPECT_EQ(0, total_video_delay_ms);
  
  
  EXPECT_EQ(current_extra_delay_ms +
            MaxAudioDelayDecrease(current_audio_delay_ms, delay_ms),
            extra_audio_delay_ms);
}

TEST_F(StreamSynchronizationTest, BothDelayedVideoLater) {
  BothDelayedVideoLaterTest(0);
}

TEST_F(StreamSynchronizationTest, BothDelayedVideoLaterAudioClockDrift) {
  audio_clock_drift_ = 1.05;
  BothDelayedVideoLaterTest(0);
}

TEST_F(StreamSynchronizationTest, BothDelayedVideoLaterVideoClockDrift) {
  video_clock_drift_ = 1.05;
  BothDelayedVideoLaterTest(0);
}

TEST_F(StreamSynchronizationTest, BothDelayedAudioLater) {
  BothDelayedAudioLaterTest(0);
}

TEST_F(StreamSynchronizationTest, BothDelayedAudioClockDrift) {
  audio_clock_drift_ = 1.05;
  BothDelayedAudioLaterTest(0);
}

TEST_F(StreamSynchronizationTest, BothDelayedVideoClockDrift) {
  video_clock_drift_ = 1.05;
  BothDelayedAudioLaterTest(0);
}

TEST_F(StreamSynchronizationTest, BaseDelay) {
  int base_target_delay_ms = 2000;
  int current_audio_delay_ms = 2000;
  int extra_audio_delay_ms = 0;
  int total_video_delay_ms = base_target_delay_ms;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  
  EXPECT_FALSE(DelayedStreams(base_target_delay_ms, base_target_delay_ms,
                              current_audio_delay_ms,
                              &extra_audio_delay_ms, &total_video_delay_ms));
  
  base_target_delay_ms = 2000;
  current_audio_delay_ms = base_target_delay_ms;
  total_video_delay_ms = base_target_delay_ms;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  
  EXPECT_FALSE(DelayedStreams(base_target_delay_ms, base_target_delay_ms,
                              current_audio_delay_ms,
                              &extra_audio_delay_ms, &total_video_delay_ms));
  
  
  base_target_delay_ms = 5000;
  current_audio_delay_ms = base_target_delay_ms;
  total_video_delay_ms = base_target_delay_ms;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  
  EXPECT_FALSE(DelayedStreams(base_target_delay_ms, base_target_delay_ms,
                              current_audio_delay_ms,
                              &extra_audio_delay_ms, &total_video_delay_ms));
}

TEST_F(StreamSynchronizationTest, BothDelayedAudioLaterWithBaseDelay) {
  int base_target_delay_ms = 3000;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  BothDelayedAudioLaterTest(base_target_delay_ms);
}

TEST_F(StreamSynchronizationTest, BothDelayedAudioClockDriftWithBaseDelay) {
  int base_target_delay_ms = 3000;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  audio_clock_drift_ = 1.05;
  BothDelayedAudioLaterTest(base_target_delay_ms);
}

TEST_F(StreamSynchronizationTest, BothDelayedVideoClockDriftWithBaseDelay) {
  int base_target_delay_ms = 3000;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  video_clock_drift_ = 1.05;
  BothDelayedAudioLaterTest(base_target_delay_ms);
}

TEST_F(StreamSynchronizationTest, BothDelayedVideoLaterWithBaseDelay) {
  int base_target_delay_ms = 2000;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  BothDelayedVideoLaterTest(base_target_delay_ms);
}

TEST_F(StreamSynchronizationTest,
       BothDelayedVideoLaterAudioClockDriftWithBaseDelay) {
  int base_target_delay_ms = 2000;
  audio_clock_drift_ = 1.05;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  BothDelayedVideoLaterTest(base_target_delay_ms);
}

TEST_F(StreamSynchronizationTest,
       BothDelayedVideoLaterVideoClockDriftWithBaseDelay) {
  int base_target_delay_ms = 2000;
  video_clock_drift_ = 1.05;
  sync_->SetTargetBufferingDelay(base_target_delay_ms);
  BothDelayedVideoLaterTest(base_target_delay_ms);
}

}  
