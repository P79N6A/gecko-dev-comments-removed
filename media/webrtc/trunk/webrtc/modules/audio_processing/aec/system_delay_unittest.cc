









#include "gtest/gtest.h"

#include "modules/audio_processing/aec/include/echo_cancellation.h"
#include "modules/audio_processing/aec/echo_cancellation_internal.h"
#include "typedefs.h"

namespace {

class SystemDelayTest : public ::testing::Test {
 protected:
  SystemDelayTest();
  virtual void SetUp();
  virtual void TearDown();

  
  
  void Init(int sample_rate_hz);

  
  void RenderAndCapture(int device_buffer_ms);

  
  int BufferFillUp();

  
  void RunStableStartup();

  
  
  int MapBufferSizeToSamples(int size_in_ms);

  void* handle_;
  aecpc_t* self_;
  int samples_per_frame_;
  
  int16_t far_[160];
  int16_t near_[160];
  int16_t out_[160];
};

SystemDelayTest::SystemDelayTest()
    : handle_(NULL),
      self_(NULL),
      samples_per_frame_(0) {
  
  memset(far_, 1, sizeof(far_));
  memset(near_, 2, sizeof(near_));
  memset(out_, 0, sizeof(out_));
}

void SystemDelayTest::SetUp() {
  ASSERT_EQ(0, WebRtcAec_Create(&handle_));
  self_ = reinterpret_cast<aecpc_t*>(handle_);
}

void SystemDelayTest::TearDown() {
  
  ASSERT_EQ(0, WebRtcAec_Free(handle_));
  handle_ = NULL;
}



static const int kSampleRateHz[] = { 8000, 16000 };
static const size_t kNumSampleRates =
    sizeof(kSampleRateHz) / sizeof(*kSampleRateHz);


static const int kDeviceBufMs = 100;



static const int kStableConvergenceMs = 100;




static const int kMaxConvergenceMs = 500;

void SystemDelayTest::Init(int sample_rate_hz) {
  
  EXPECT_EQ(0, WebRtcAec_Init(handle_, sample_rate_hz, 48000));

  
  samples_per_frame_ = sample_rate_hz / 100;
}

void SystemDelayTest::RenderAndCapture(int device_buffer_ms) {
  EXPECT_EQ(0, WebRtcAec_BufferFarend(handle_, far_, samples_per_frame_));
  EXPECT_EQ(0, WebRtcAec_Process(handle_, near_, NULL, out_, NULL,
                                 samples_per_frame_, device_buffer_ms, 0));
}

int SystemDelayTest::BufferFillUp() {
  
  
  
  int buffer_size = 0;
  for (int i = 0; i < kDeviceBufMs / 10; i++) {
    EXPECT_EQ(0, WebRtcAec_BufferFarend(handle_, far_, samples_per_frame_));
    buffer_size += samples_per_frame_;
    EXPECT_EQ(buffer_size, self_->aec->system_delay);
  }
  return buffer_size;
}

void SystemDelayTest::RunStableStartup() {
  
  
  
  int buffer_size = BufferFillUp();
  
  
  int process_time_ms = 0;
  for (; process_time_ms < kStableConvergenceMs; process_time_ms += 10) {
    RenderAndCapture(kDeviceBufMs);
    buffer_size += samples_per_frame_;
    if (self_->ECstartup == 0) {
      
      break;
    }
  }
  
  EXPECT_GT(kStableConvergenceMs, process_time_ms);
  
  EXPECT_GE(buffer_size, self_->aec->system_delay);
}

int SystemDelayTest::MapBufferSizeToSamples(int size_in_ms) {
  
  return (size_in_ms + 10) * samples_per_frame_ / 10;
}






















TEST_F(SystemDelayTest, CorrectIncreaseWhenBufferFarend) {
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);

    
    
    for (int j = 1; j <= 5; j++) {
      EXPECT_EQ(0, WebRtcAec_BufferFarend(handle_, far_, samples_per_frame_));
      EXPECT_EQ(j * samples_per_frame_, self_->aec->system_delay);
    }
  }
}




TEST_F(SystemDelayTest, CorrectDelayAfterStableStartup) {
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);
    RunStableStartup();

    
    
    
    int average_reported_delay = kDeviceBufMs * samples_per_frame_ / 10;
    EXPECT_GE(average_reported_delay, self_->aec->system_delay);
    EXPECT_LE(average_reported_delay * 3 / 4, self_->aec->system_delay);
  }
}

TEST_F(SystemDelayTest, CorrectDelayAfterUnstableStartup) {
  
  
  
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);

    
    
    
    int buffer_size = BufferFillUp();

    int buffer_offset_ms = 25;
    int reported_delay_ms = 0;
    int process_time_ms = 0;
    for (; process_time_ms <= kMaxConvergenceMs; process_time_ms += 10) {
      reported_delay_ms = kDeviceBufMs + buffer_offset_ms;
      RenderAndCapture(reported_delay_ms);
      buffer_size += samples_per_frame_;
      buffer_offset_ms = -buffer_offset_ms;
      if (self_->ECstartup == 0) {
        
        break;
      }
    }
    
    EXPECT_GE(kMaxConvergenceMs, process_time_ms);
    
    EXPECT_GE(buffer_size, self_->aec->system_delay);

    
    
    EXPECT_GE(reported_delay_ms * samples_per_frame_ / 10,
              self_->aec->system_delay);
    EXPECT_LE(reported_delay_ms * samples_per_frame_ / 10 * 3 / 5,
              self_->aec->system_delay);
  }
}

TEST_F(SystemDelayTest, CorrectDelayAfterStableBufferBuildUp) {
  
  
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);

    
    
    
    int process_time_ms = 0;
    for (; process_time_ms < kStableConvergenceMs; process_time_ms += 10) {
      EXPECT_EQ(0, WebRtcAec_Process(handle_, near_, NULL, out_, NULL,
                                     samples_per_frame_, kDeviceBufMs, 0));
    }
    
    EXPECT_EQ(0, self_->checkBuffSize);

    
    
    int buffer_size = 0;
    int target_buffer_size = kDeviceBufMs * samples_per_frame_ / 10 * 3 / 4;
    process_time_ms = 0;
    for (; process_time_ms <= kMaxConvergenceMs; process_time_ms += 10) {
      RenderAndCapture(kDeviceBufMs);
      buffer_size += samples_per_frame_;
      if (self_->ECstartup == 0) {
        
        break;
      }
    }
    
    EXPECT_GT(kMaxConvergenceMs, process_time_ms);
    
    EXPECT_LE(target_buffer_size, self_->aec->system_delay);

    
    
    for (int j = 0; j < 6; j++) {
      int system_delay_before_calls = self_->aec->system_delay;
      RenderAndCapture(kDeviceBufMs);
      EXPECT_EQ(system_delay_before_calls, self_->aec->system_delay);
    }
  }
}

TEST_F(SystemDelayTest, CorrectDelayWhenBufferUnderrun) {
  
  
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);
    RunStableStartup();

    
    
    
    for (int j = 0; j <= kStableConvergenceMs; j += 10) {
      EXPECT_EQ(0, WebRtcAec_Process(handle_, near_, NULL, out_, NULL,
                                     samples_per_frame_, kDeviceBufMs, 0));
      EXPECT_LE(0, self_->aec->system_delay);
    }
  }
}

TEST_F(SystemDelayTest, CorrectDelayDuringDrift) {
  
  
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);
    RunStableStartup();

    
    int jump = 0;
    for (int j = 0; j < 1000; j++) {
      
      int device_buf_ms = kDeviceBufMs - (j / 10) + jump;
      int device_buf = MapBufferSizeToSamples(device_buf_ms);

      if (device_buf_ms < 30) {
        
        jump += 10;
      }
      RenderAndCapture(device_buf_ms);

      
      EXPECT_GE(device_buf, self_->aec->system_delay);

      
      EXPECT_LE(0, self_->aec->system_delay);
    }
  }
}

TEST_F(SystemDelayTest, ShouldRecoverAfterGlitch) {
  
  
  
  
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);
    RunStableStartup();
    int device_buf = MapBufferSizeToSamples(kDeviceBufMs);
    
    for (int j = 0; j < 20; j++) {
      EXPECT_EQ(0, WebRtcAec_BufferFarend(handle_, far_, samples_per_frame_));
      
    }
    
    
    EXPECT_LT(device_buf, self_->aec->system_delay);

    
    
    bool non_causal = true;  
    for (int j = 0; j < 50; j++) {
      int system_delay_before = self_->aec->system_delay;
      RenderAndCapture(kDeviceBufMs);
      int system_delay_after = self_->aec->system_delay;

      
      
      
      if (non_causal) {
        EXPECT_LT(system_delay_after, system_delay_before);
        if (device_buf - system_delay_after >= 64) {
          non_causal = false;
        }
      } else {
        EXPECT_EQ(system_delay_before, system_delay_after);
      }
      
      EXPECT_LE(0, self_->aec->system_delay);
    }
    
    EXPECT_FALSE(non_causal);
  }
}

TEST_F(SystemDelayTest, UnaffectedWhenSpuriousDeviceBufferValues) {
  
  
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);
    RunStableStartup();
    int device_buf = MapBufferSizeToSamples(kDeviceBufMs);

    
    bool non_causal = false;

    
    for (int j = 0; j < 100; j++) {
      int system_delay_before_calls = self_->aec->system_delay;
      int device_buf_ms = kDeviceBufMs;
      if (j % 10 == 0) {
        device_buf_ms = 500;
      }
      RenderAndCapture(device_buf_ms);

      
      if (device_buf - self_->aec->system_delay < 64) {
        non_causal = true;
      }
      EXPECT_FALSE(non_causal);
      EXPECT_EQ(system_delay_before_calls, self_->aec->system_delay);

      
      EXPECT_LE(0, self_->aec->system_delay);
    }
  }
}

TEST_F(SystemDelayTest, CorrectImpactWhenTogglingDeviceBufferValues) {
  
  
  
  
  
  
  
  
  
  
  for (size_t i = 0; i < kNumSampleRates; i++) {
    Init(kSampleRateHz[i]);
    RunStableStartup();
    int device_buf = MapBufferSizeToSamples(kDeviceBufMs);

    
    bool non_causal = false;

    
    
    
    for (int j = 0; j < 100; j++) {
      int system_delay_before_calls = self_->aec->system_delay;
      int device_buf_ms = 2 * (j % 2) * kDeviceBufMs;
      RenderAndCapture(device_buf_ms);

      
      non_causal |= (device_buf - self_->aec->system_delay < 64);
      EXPECT_GE(system_delay_before_calls, self_->aec->system_delay);

      
      EXPECT_LE(0, self_->aec->system_delay);
    }
    
    EXPECT_FALSE(non_causal);
  }
}

}  
