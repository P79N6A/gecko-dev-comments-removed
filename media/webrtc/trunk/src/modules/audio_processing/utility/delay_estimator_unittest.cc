









#include "gtest/gtest.h"

extern "C" {
#include "modules/audio_processing/utility/delay_estimator.h"
#include "modules/audio_processing/utility/delay_estimator_internal.h"
#include "modules/audio_processing/utility/delay_estimator_wrapper.h"
}
#include "typedefs.h"

namespace {

enum { kSpectrumSize = 65 };

enum { kMaxDelay = 100 };
enum { kLookahead = 10 };

class DelayEstimatorTest : public ::testing::Test {
 protected:
  DelayEstimatorTest();
  virtual void SetUp();
  virtual void TearDown();

  void Init();

  void InitBinary();

  void* handle_;
  DelayEstimator* self_;
  BinaryDelayEstimator* binary_handle_;
  int spectrum_size_;
  
  float far_f_[kSpectrumSize];
  float near_f_[kSpectrumSize];
  uint16_t far_u16_[kSpectrumSize];
  uint16_t near_u16_[kSpectrumSize];
};

DelayEstimatorTest::DelayEstimatorTest()
    : handle_(NULL),
      self_(NULL),
      binary_handle_(NULL),
      spectrum_size_(kSpectrumSize) {
  
  memset(far_f_, 1, sizeof(far_f_));
  memset(near_f_, 2, sizeof(near_f_));
  memset(far_u16_, 1, sizeof(far_u16_));
  memset(near_u16_, 2, sizeof(near_u16_));
}

void DelayEstimatorTest::SetUp() {
  handle_ = WebRtc_CreateDelayEstimator(kSpectrumSize, kMaxDelay, kLookahead);
  ASSERT_TRUE(handle_ != NULL);
  self_ = reinterpret_cast<DelayEstimator*>(handle_);
  binary_handle_ = self_->binary_handle;
}

void DelayEstimatorTest::TearDown() {
  WebRtc_FreeDelayEstimator(handle_);
  handle_ = NULL;
  self_ = NULL;
  binary_handle_ = NULL;
}

void DelayEstimatorTest::Init() {
  
  EXPECT_EQ(0, WebRtc_InitDelayEstimator(handle_));
  
  EXPECT_EQ(0, self_->far_spectrum_initialized);
  EXPECT_EQ(0, self_->near_spectrum_initialized);
}

void DelayEstimatorTest::InitBinary() {
  
  WebRtc_InitBinaryDelayEstimator(binary_handle_);
  
  
  EXPECT_EQ(-2, binary_handle_->last_delay);
}

TEST_F(DelayEstimatorTest, CorrectErrorReturnsOfWrapper) {
  

  
  
  
  
  void* handle = handle_;
  handle = WebRtc_CreateDelayEstimator(33, kMaxDelay, kLookahead);
  EXPECT_TRUE(handle == NULL);
  handle = handle_;
  handle = WebRtc_CreateDelayEstimator(kSpectrumSize, -1, kLookahead);
  EXPECT_TRUE(handle == NULL);
  handle = handle_;
  handle = WebRtc_CreateDelayEstimator(kSpectrumSize, kMaxDelay, -1);
  EXPECT_TRUE(handle == NULL);
  handle = handle_;
  handle = WebRtc_CreateDelayEstimator(kSpectrumSize, 0, 0);
  EXPECT_TRUE(handle == NULL);

  
  
  EXPECT_EQ(-1, WebRtc_InitDelayEstimator(NULL));

  
  
  
  
  
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFloat(NULL, far_f_, near_f_,
                                                  spectrum_size_));
  
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFloat(handle_, NULL, near_f_,
                                                  spectrum_size_));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFloat(handle_, far_f_, NULL,
                                                  spectrum_size_));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFloat(handle_, far_f_, near_f_,
                                                  spectrum_size_ + 1));

  
  
  
  
  
  
  
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(NULL, far_u16_, near_u16_,
                                                spectrum_size_, 0, 0));
  
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(handle_, NULL, near_u16_,
                                                spectrum_size_, 0, 0));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(handle_, far_u16_, NULL,
                                                spectrum_size_, 0, 0));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(handle_, far_u16_, near_u16_,
                                                spectrum_size_ + 1, 0, 0));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(handle_, far_u16_, near_u16_,
                                                spectrum_size_, 16, 0));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(handle_, far_u16_, near_u16_,
                                                spectrum_size_, 0, 16));

  
  EXPECT_EQ(-1, WebRtc_last_delay(NULL));

  
  WebRtc_FreeDelayEstimator(handle);
}

TEST_F(DelayEstimatorTest, InitializedSpectrumAfterProcess) {
  
  

  
  
  Init();
  EXPECT_EQ(-2, WebRtc_DelayEstimatorProcessFloat(handle_, far_f_, near_f_,
                                                  spectrum_size_));
  EXPECT_EQ(1, self_->far_spectrum_initialized);
  EXPECT_EQ(1, self_->near_spectrum_initialized);

  
  
  Init();
  EXPECT_EQ(-2, WebRtc_DelayEstimatorProcessFix(handle_, far_u16_, near_u16_,
                                                spectrum_size_, 0, 0));
  EXPECT_EQ(1, self_->far_spectrum_initialized);
  EXPECT_EQ(1, self_->near_spectrum_initialized);
}

TEST_F(DelayEstimatorTest, CorrectLastDelay) {
  
  
  
  

  int last_delay = 0;
  
  Init();
  for (int i = 0; i < 200; i++) {
    last_delay = WebRtc_DelayEstimatorProcessFloat(handle_, far_f_, near_f_,
                                                   spectrum_size_);
    if (last_delay != -2) {
      EXPECT_EQ(last_delay, WebRtc_last_delay(handle_));
      break;
    }
  }
  
  EXPECT_NE(-2, WebRtc_last_delay(handle_));

  
  Init();
  for (int i = 0; i < 200; i++) {
    last_delay = WebRtc_DelayEstimatorProcessFix(handle_, far_u16_, near_u16_,
                                                 spectrum_size_, 0, 0);
    if (last_delay != -2) {
      EXPECT_EQ(last_delay, WebRtc_last_delay(handle_));
      break;
    }
  }
  
  EXPECT_NE(-2, WebRtc_last_delay(handle_));
}

TEST_F(DelayEstimatorTest, CorrectErrorReturnsOfBinaryEstimator) {
  
  

  BinaryDelayEstimator* binary_handle = binary_handle_;
  
  
  
  
  
  binary_handle = WebRtc_CreateBinaryDelayEstimator(-1, kLookahead);
  EXPECT_TRUE(binary_handle == NULL);
  binary_handle = binary_handle_;
  binary_handle = WebRtc_CreateBinaryDelayEstimator(kMaxDelay, -1);
  EXPECT_TRUE(binary_handle == NULL);
  binary_handle = binary_handle_;
  binary_handle = WebRtc_CreateBinaryDelayEstimator(0, 0);
  EXPECT_TRUE(binary_handle == NULL);

  
  
  
  
  

  
}

TEST_F(DelayEstimatorTest, MeanEstimatorFix) {
  
  

  InitBinary();

  int32_t mean_value = 4000;
  int32_t mean_value_before = mean_value;
  int32_t new_mean_value = mean_value * 2;

  
  WebRtc_MeanEstimatorFix(new_mean_value, 10, &mean_value);
  EXPECT_LT(mean_value_before, mean_value);
  EXPECT_GT(new_mean_value, mean_value);

  
  new_mean_value = mean_value / 2;
  mean_value_before = mean_value;
  WebRtc_MeanEstimatorFix(new_mean_value, 10, &mean_value);
  EXPECT_GT(mean_value_before, mean_value);
  EXPECT_LT(new_mean_value, mean_value);
}

TEST_F(DelayEstimatorTest, ExactDelayEstimate) {
  
  

  
  
  
  const int sequence_length = 400;
  uint32_t binary_spectrum[sequence_length + kMaxDelay + kLookahead];
  binary_spectrum[0] = 1;
  for (int i = 1; i < (sequence_length + kMaxDelay + kLookahead); i++) {
    binary_spectrum[i] = 3 * binary_spectrum[i - 1];
  }

  
  
  
  
  for (int offset = -kLookahead; offset < kMaxDelay; offset++) {
    InitBinary();
    for (int i = kLookahead; i < (sequence_length + kLookahead); i++) {
      int delay = WebRtc_ProcessBinarySpectrum(binary_handle_,
                                               binary_spectrum[i + offset],
                                               binary_spectrum[i]);

      
      EXPECT_EQ(delay, WebRtc_binary_last_delay(binary_handle_));

      if (delay != -2) {
        
        
        EXPECT_EQ(offset, delay - kLookahead);
      }
    }
    
    EXPECT_NE(-2, WebRtc_binary_last_delay(binary_handle_));
  }
}

}  
