









#include "testing/gtest/include/gtest/gtest.h"

extern "C" {
#include "webrtc/modules/audio_processing/utility/delay_estimator.h"
#include "webrtc/modules/audio_processing/utility/delay_estimator_internal.h"
#include "webrtc/modules/audio_processing/utility/delay_estimator_wrapper.h"
}
#include "webrtc/typedefs.h"

namespace {

enum { kSpectrumSize = 65 };

enum { kMaxDelay = 100 };
enum { kLookahead = 10 };

enum { kSequenceLength = 400 };

const int kEnable[] = { 0, 1 };
const size_t kSizeEnable = sizeof(kEnable) / sizeof(*kEnable);

class DelayEstimatorTest : public ::testing::Test {
 protected:
  DelayEstimatorTest();
  virtual void SetUp();
  virtual void TearDown();

  void Init();
  void InitBinary();
  void VerifyDelay(BinaryDelayEstimator* binary_handle, int offset, int delay);
  void RunBinarySpectra(BinaryDelayEstimator* binary1,
                        BinaryDelayEstimator* binary2,
                        int near_offset, int lookahead_offset, int far_offset);
  void RunBinarySpectraTest(int near_offset, int lookahead_offset,
                            int ref_robust_validation, int robust_validation);

  void* handle_;
  DelayEstimator* self_;
  void* farend_handle_;
  DelayEstimatorFarend* farend_self_;
  BinaryDelayEstimator* binary_;
  BinaryDelayEstimatorFarend* binary_farend_;
  int spectrum_size_;
  
  float far_f_[kSpectrumSize];
  float near_f_[kSpectrumSize];
  uint16_t far_u16_[kSpectrumSize];
  uint16_t near_u16_[kSpectrumSize];
  uint32_t binary_spectrum_[kSequenceLength + kMaxDelay + kLookahead];
};

DelayEstimatorTest::DelayEstimatorTest()
    : handle_(NULL),
      self_(NULL),
      farend_handle_(NULL),
      farend_self_(NULL),
      binary_(NULL),
      binary_farend_(NULL),
      spectrum_size_(kSpectrumSize) {
  
  memset(far_f_, 1, sizeof(far_f_));
  memset(near_f_, 2, sizeof(near_f_));
  memset(far_u16_, 1, sizeof(far_u16_));
  memset(near_u16_, 2, sizeof(near_u16_));
  
  
  
  binary_spectrum_[0] = 1;
  for (int i = 1; i < (kSequenceLength + kMaxDelay + kLookahead); i++) {
    binary_spectrum_[i] = 3 * binary_spectrum_[i - 1];
  }
}

void DelayEstimatorTest::SetUp() {
  farend_handle_ = WebRtc_CreateDelayEstimatorFarend(kSpectrumSize,
                                                     kMaxDelay + kLookahead);
  ASSERT_TRUE(farend_handle_ != NULL);
  farend_self_ = reinterpret_cast<DelayEstimatorFarend*>(farend_handle_);
  handle_ = WebRtc_CreateDelayEstimator(farend_handle_, kLookahead);
  ASSERT_TRUE(handle_ != NULL);
  self_ = reinterpret_cast<DelayEstimator*>(handle_);
  binary_farend_ = WebRtc_CreateBinaryDelayEstimatorFarend(kMaxDelay +
                                                           kLookahead);
  ASSERT_TRUE(binary_farend_ != NULL);
  binary_ = WebRtc_CreateBinaryDelayEstimator(binary_farend_, kLookahead);
  ASSERT_TRUE(binary_ != NULL);
}

void DelayEstimatorTest::TearDown() {
  WebRtc_FreeDelayEstimator(handle_);
  handle_ = NULL;
  self_ = NULL;
  WebRtc_FreeDelayEstimatorFarend(farend_handle_);
  farend_handle_ = NULL;
  farend_self_ = NULL;
  WebRtc_FreeBinaryDelayEstimator(binary_);
  binary_ = NULL;
  WebRtc_FreeBinaryDelayEstimatorFarend(binary_farend_);
  binary_farend_ = NULL;
}

void DelayEstimatorTest::Init() {
  
  EXPECT_EQ(0, WebRtc_InitDelayEstimatorFarend(farend_handle_));
  EXPECT_EQ(0, WebRtc_InitDelayEstimator(handle_));
  
  EXPECT_EQ(0, farend_self_->far_spectrum_initialized);
  EXPECT_EQ(0, self_->near_spectrum_initialized);
  EXPECT_EQ(-2, WebRtc_last_delay(handle_));  
  EXPECT_EQ(0, WebRtc_last_delay_quality(handle_));  
}

void DelayEstimatorTest::InitBinary() {
  
  WebRtc_InitBinaryDelayEstimatorFarend(binary_farend_);
  
  WebRtc_InitBinaryDelayEstimator(binary_);
  
  
  EXPECT_EQ(-2, binary_->last_delay);
}

void DelayEstimatorTest::VerifyDelay(BinaryDelayEstimator* binary_handle,
                                     int offset, int delay) {
  
  EXPECT_EQ(delay, WebRtc_binary_last_delay(binary_handle));

  if (delay != -2) {
    
    
    EXPECT_EQ(offset, delay);
  }
}

void DelayEstimatorTest::RunBinarySpectra(BinaryDelayEstimator* binary1,
                                          BinaryDelayEstimator* binary2,
                                          int near_offset,
                                          int lookahead_offset,
                                          int far_offset) {
  int different_validations = binary1->robust_validation_enabled ^
      binary2->robust_validation_enabled;
  WebRtc_InitBinaryDelayEstimatorFarend(binary_farend_);
  WebRtc_InitBinaryDelayEstimator(binary1);
  WebRtc_InitBinaryDelayEstimator(binary2);
  
  
  EXPECT_EQ(-2, binary1->last_delay);
  EXPECT_EQ(-2, binary2->last_delay);
  for (int i = kLookahead; i < (kSequenceLength + kLookahead); i++) {
    WebRtc_AddBinaryFarSpectrum(binary_farend_,
                                binary_spectrum_[i + far_offset]);
    int delay_1 = WebRtc_ProcessBinarySpectrum(binary1, binary_spectrum_[i]);
    int delay_2 =
        WebRtc_ProcessBinarySpectrum(binary2,
                                     binary_spectrum_[i - near_offset]);

    VerifyDelay(binary1, far_offset + kLookahead, delay_1);
    VerifyDelay(binary2,
                far_offset + kLookahead + lookahead_offset + near_offset,
                delay_2);
    
    
    if ((delay_1 != -2) && (delay_2 != -2)) {
      EXPECT_EQ(delay_1, delay_2 - lookahead_offset - near_offset);
    }
    
    
    
    if ((near_offset == 0) && (lookahead_offset == 0)) {
      if  (!different_validations) {
        EXPECT_EQ(delay_1, delay_2);
      } else {
        if (binary1->robust_validation_enabled) {
          EXPECT_GE(delay_1, delay_2);
        } else {
          EXPECT_GE(delay_2, delay_1);
        }
      }
    }
  }
  
  EXPECT_NE(-2, WebRtc_binary_last_delay(binary1));
  EXPECT_NE(0, WebRtc_binary_last_delay_quality(binary1));
  EXPECT_NE(-2, WebRtc_binary_last_delay(binary2));
  EXPECT_NE(0, WebRtc_binary_last_delay_quality(binary2));
}

void DelayEstimatorTest::RunBinarySpectraTest(int near_offset,
                                              int lookahead_offset,
                                              int ref_robust_validation,
                                              int robust_validation) {
  BinaryDelayEstimator* binary2 =
      WebRtc_CreateBinaryDelayEstimator(binary_farend_,
                                        kLookahead + lookahead_offset);
  
  
  
  
  binary_->robust_validation_enabled = ref_robust_validation;
  binary2->robust_validation_enabled = robust_validation;
  for (int offset = -kLookahead;
      offset < kMaxDelay - lookahead_offset - near_offset;
      offset++) {
    RunBinarySpectra(binary_, binary2, near_offset, lookahead_offset, offset);
  }
  WebRtc_FreeBinaryDelayEstimator(binary2);
  binary2 = NULL;
  binary_->robust_validation_enabled = 0;  
}

TEST_F(DelayEstimatorTest, CorrectErrorReturnsOfWrapper) {
  

  
  
  
  
  void* handle = farend_handle_;
  handle = WebRtc_CreateDelayEstimatorFarend(33, kMaxDelay + kLookahead);
  EXPECT_TRUE(handle == NULL);
  handle = farend_handle_;
  handle = WebRtc_CreateDelayEstimatorFarend(kSpectrumSize, 1);
  EXPECT_TRUE(handle == NULL);

  handle = handle_;
  handle = WebRtc_CreateDelayEstimator(NULL, kLookahead);
  EXPECT_TRUE(handle == NULL);
  handle = handle_;
  handle = WebRtc_CreateDelayEstimator(farend_handle_, -1);
  EXPECT_TRUE(handle == NULL);

  
  
  EXPECT_EQ(-1, WebRtc_InitDelayEstimatorFarend(NULL));
  EXPECT_EQ(-1, WebRtc_InitDelayEstimator(NULL));

  
  
  
  
  EXPECT_EQ(-1, WebRtc_AddFarSpectrumFloat(NULL, far_f_, spectrum_size_));
  
  EXPECT_EQ(-1, WebRtc_AddFarSpectrumFloat(farend_handle_, NULL,
                                           spectrum_size_));
  EXPECT_EQ(-1, WebRtc_AddFarSpectrumFloat(farend_handle_, far_f_,
                                           spectrum_size_ + 1));

  
  
  
  
  
  EXPECT_EQ(-1, WebRtc_AddFarSpectrumFix(NULL, far_u16_, spectrum_size_, 0));
  EXPECT_EQ(-1, WebRtc_AddFarSpectrumFix(farend_handle_, NULL, spectrum_size_,
                                         0));
  EXPECT_EQ(-1, WebRtc_AddFarSpectrumFix(farend_handle_, far_u16_,
                                         spectrum_size_ + 1, 0));
  EXPECT_EQ(-1, WebRtc_AddFarSpectrumFix(farend_handle_, far_u16_,
                                         spectrum_size_, 16));

  
  
  
  EXPECT_EQ(-1, WebRtc_set_allowed_offset(NULL, 0));
  EXPECT_EQ(-1, WebRtc_set_allowed_offset(handle_, -1));

  EXPECT_EQ(-1, WebRtc_get_allowed_offset(NULL));

  
  
  
  EXPECT_EQ(-1, WebRtc_enable_robust_validation(NULL, kEnable[0]));
  EXPECT_EQ(-1, WebRtc_enable_robust_validation(handle_, -1));
  EXPECT_EQ(-1, WebRtc_enable_robust_validation(handle_, 2));

  
  
  EXPECT_EQ(-1, WebRtc_is_robust_validation_enabled(NULL));

  
  
  
  
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFloat(NULL, near_f_,
                                                  spectrum_size_));
  
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFloat(handle_, NULL,
                                                  spectrum_size_));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFloat(handle_, near_f_,
                                                  spectrum_size_ + 1));

  
  
  
  
  
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(NULL, near_u16_, spectrum_size_,
                                                0));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(handle_, NULL, spectrum_size_,
                                                0));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(handle_, near_u16_,
                                                spectrum_size_ + 1, 0));
  EXPECT_EQ(-1, WebRtc_DelayEstimatorProcessFix(handle_, near_u16_,
                                                spectrum_size_, 16));

  
  EXPECT_EQ(-1, WebRtc_last_delay(NULL));

  
  
  EXPECT_EQ(-1, WebRtc_last_delay_quality(NULL));

  
  WebRtc_FreeDelayEstimator(handle);
}

TEST_F(DelayEstimatorTest, VerifyAllowedOffset) {
  
  EXPECT_EQ(0, WebRtc_get_allowed_offset(handle_));
  for (int i = 1; i >= 0; i--) {
    EXPECT_EQ(0, WebRtc_set_allowed_offset(handle_, i));
    EXPECT_EQ(i, WebRtc_get_allowed_offset(handle_));
    Init();
    
    EXPECT_EQ(i, WebRtc_get_allowed_offset(handle_));
  }
}

TEST_F(DelayEstimatorTest, VerifyEnableRobustValidation) {
  
  EXPECT_EQ(0, WebRtc_is_robust_validation_enabled(handle_));
  for (size_t i = 0; i < kSizeEnable; ++i) {
    EXPECT_EQ(0, WebRtc_enable_robust_validation(handle_, kEnable[i]));
    EXPECT_EQ(kEnable[i], WebRtc_is_robust_validation_enabled(handle_));
    Init();
    
    EXPECT_EQ(kEnable[i], WebRtc_is_robust_validation_enabled(handle_));
  }
}

TEST_F(DelayEstimatorTest, InitializedSpectrumAfterProcess) {
  
  

  
  
  Init();
  EXPECT_EQ(0, WebRtc_AddFarSpectrumFloat(farend_handle_, far_f_,
                                           spectrum_size_));
  EXPECT_EQ(1, farend_self_->far_spectrum_initialized);
  EXPECT_EQ(-2, WebRtc_DelayEstimatorProcessFloat(handle_, near_f_,
                                                  spectrum_size_));
  EXPECT_EQ(1, self_->near_spectrum_initialized);

  
  
  Init();
  EXPECT_EQ(0, WebRtc_AddFarSpectrumFix(farend_handle_, far_u16_,
                                         spectrum_size_, 0));
  EXPECT_EQ(1, farend_self_->far_spectrum_initialized);
  EXPECT_EQ(-2, WebRtc_DelayEstimatorProcessFix(handle_, near_u16_,
                                                spectrum_size_, 0));
  EXPECT_EQ(1, self_->near_spectrum_initialized);
}

TEST_F(DelayEstimatorTest, CorrectLastDelay) {
  
  
  
  

  int last_delay = 0;
  
  Init();
  for (int i = 0; i < 200; i++) {
    EXPECT_EQ(0, WebRtc_AddFarSpectrumFloat(farend_handle_, far_f_,
                                            spectrum_size_));
    last_delay = WebRtc_DelayEstimatorProcessFloat(handle_, near_f_,
                                                   spectrum_size_);
    if (last_delay != -2) {
      EXPECT_EQ(last_delay, WebRtc_last_delay(handle_));
      EXPECT_EQ(7203, WebRtc_last_delay_quality(handle_));
      break;
    }
  }
  
  EXPECT_NE(-2, WebRtc_last_delay(handle_));
  EXPECT_NE(0, WebRtc_last_delay_quality(handle_));

  
  Init();
  for (int i = 0; i < 200; i++) {
    EXPECT_EQ(0, WebRtc_AddFarSpectrumFix(farend_handle_, far_u16_,
                                          spectrum_size_, 0));
    last_delay = WebRtc_DelayEstimatorProcessFix(handle_, near_u16_,
                                                 spectrum_size_, 0);
    if (last_delay != -2) {
      EXPECT_EQ(last_delay, WebRtc_last_delay(handle_));
      EXPECT_EQ(7203, WebRtc_last_delay_quality(handle_));
      break;
    }
  }
  
  EXPECT_NE(-2, WebRtc_last_delay(handle_));
  EXPECT_NE(0, WebRtc_last_delay_quality(handle_));
}

TEST_F(DelayEstimatorTest, CorrectErrorReturnsOfBinaryEstimatorFarend) {
  
  

  BinaryDelayEstimatorFarend* binary = binary_farend_;
  
  
  
  
  
  binary = WebRtc_CreateBinaryDelayEstimatorFarend(1);
  EXPECT_TRUE(binary == NULL);
}

TEST_F(DelayEstimatorTest, CorrectErrorReturnsOfBinaryEstimator) {
  
  

  BinaryDelayEstimator* binary_handle = binary_;
  
  
  
  
  
  binary_handle = WebRtc_CreateBinaryDelayEstimator(NULL, kLookahead);
  EXPECT_TRUE(binary_handle == NULL);
  binary_handle = binary_;
  binary_handle = WebRtc_CreateBinaryDelayEstimator(binary_farend_, -1);
  EXPECT_TRUE(binary_handle == NULL);
}

TEST_F(DelayEstimatorTest, MeanEstimatorFix) {
  
  

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

TEST_F(DelayEstimatorTest, ExactDelayEstimateMultipleNearSameSpectrum) {
  
  
  
  
  
  
  

  for (size_t i = 0; i < kSizeEnable; ++i) {
    for (size_t j = 0; j < kSizeEnable; ++j) {
      RunBinarySpectraTest(0, 0, kEnable[i], kEnable[j]);
    }
  }
}

TEST_F(DelayEstimatorTest, ExactDelayEstimateMultipleNearDifferentSpectrum) {
  
  
  
  
  
  

  const int kNearOffset = 1;
  for (size_t i = 0; i < kSizeEnable; ++i) {
    for (size_t j = 0; j < kSizeEnable; ++j) {
      RunBinarySpectraTest(kNearOffset, 0, kEnable[i], kEnable[j]);
    }
  }
}

TEST_F(DelayEstimatorTest, ExactDelayEstimateMultipleNearDifferentLookahead) {
  
  
  
  
  
  

  const int kLookaheadOffset = 1;
  for (size_t i = 0; i < kSizeEnable; ++i) {
    for (size_t j = 0; j < kSizeEnable; ++j) {
      RunBinarySpectraTest(0, kLookaheadOffset, kEnable[i], kEnable[j]);
    }
  }
}

TEST_F(DelayEstimatorTest, AllowedOffsetNoImpactWhenRobustValidationDisabled) {
  
  
  

  binary_->allowed_offset = 10;
  RunBinarySpectraTest(0, 0, 0, 0);
  binary_->allowed_offset = 0;  
}

}  
