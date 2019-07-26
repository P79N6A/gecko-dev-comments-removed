











#include "webrtc/modules/audio_coding/neteq4/delay_manager.h"

#include <math.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/mock/mock_delay_peak_detector.h"

namespace webrtc {

using ::testing::Return;
using ::testing::_;

class DelayManagerTest : public ::testing::Test {
 protected:
  static const int kMaxNumberOfPackets = 240;
  static const int kTimeStepMs = 10;
  static const int kFs = 8000;
  static const int kFrameSizeMs = 20;
  static const int kTsIncrement = kFrameSizeMs * kFs / 1000;

  DelayManagerTest();
  virtual void SetUp();
  virtual void TearDown();
  void SetPacketAudioLength(int lengt_ms);
  void InsertNextPacket();
  void IncreaseTime(int inc_ms);

  DelayManager* dm_;
  MockDelayPeakDetector detector_;
  uint16_t seq_no_;
  uint32_t ts_;
};

DelayManagerTest::DelayManagerTest()
    : dm_(NULL),
      seq_no_(0x1234),
      ts_(0x12345678) {
}

void DelayManagerTest::SetUp() {
  EXPECT_CALL(detector_, Reset())
            .Times(1);
  dm_ = new DelayManager(kMaxNumberOfPackets, &detector_);
}

void DelayManagerTest::SetPacketAudioLength(int lengt_ms) {
  EXPECT_CALL(detector_, SetPacketAudioLength(lengt_ms));
  dm_->SetPacketAudioLength(lengt_ms);
}

void DelayManagerTest::InsertNextPacket() {
  EXPECT_EQ(0, dm_->Update(seq_no_, ts_, kFs));
  seq_no_ += 1;
  ts_ += kTsIncrement;
}

void DelayManagerTest::IncreaseTime(int inc_ms) {
  for (int t = 0; t < inc_ms; t += kTimeStepMs) {
    EXPECT_CALL(detector_, IncrementCounter(kTimeStepMs))
        .Times(1);
    dm_->UpdateCounters(kTimeStepMs);
  }
}
void DelayManagerTest::TearDown() {
  EXPECT_CALL(detector_, Die());
  delete dm_;
}

TEST_F(DelayManagerTest, CreateAndDestroy) {
  
  
}

TEST_F(DelayManagerTest, VectorInitialization) {
  const DelayManager::IATVector& vec = dm_->iat_vector();
  double sum = 0.0;
  for (size_t i = 0; i < vec.size(); i++) {
    EXPECT_NEAR(ldexp(pow(0.5, static_cast<int>(i + 1)), 30), vec[i], 65536);
    
    sum += vec[i];
  }
  EXPECT_EQ(1 << 30, static_cast<int>(sum));  
}

TEST_F(DelayManagerTest, SetPacketAudioLength) {
  const int kLengthMs = 30;
  
  EXPECT_CALL(detector_, SetPacketAudioLength(kLengthMs))
      .Times(1);
  EXPECT_EQ(0, dm_->SetPacketAudioLength(kLengthMs));
  EXPECT_EQ(-1, dm_->SetPacketAudioLength(-1));  
}

TEST_F(DelayManagerTest, PeakFound) {
  
  
  
  EXPECT_CALL(detector_, peak_found())
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_TRUE(dm_->PeakFound());
  EXPECT_FALSE(dm_->PeakFound());
}

TEST_F(DelayManagerTest, UpdateCounters) {
  
  EXPECT_CALL(detector_, IncrementCounter(kTimeStepMs))
      .Times(1);
  dm_->UpdateCounters(kTimeStepMs);
}

TEST_F(DelayManagerTest, UpdateNormal) {
  SetPacketAudioLength(kFrameSizeMs);
  
  InsertNextPacket();
  
  IncreaseTime(kFrameSizeMs);
  
  
  
  
  EXPECT_CALL(detector_, Update(1, 1))
      .WillOnce(Return(false));
  InsertNextPacket();
  EXPECT_EQ(1 << 8, dm_->TargetLevel());  
  EXPECT_EQ(1, dm_->base_target_level());
  int lower, higher;
  dm_->BufferLimits(&lower, &higher);
  
  
  
  EXPECT_EQ((1 << 8) * 3 / 4, lower);
  EXPECT_EQ(lower + (20 << 8) / kFrameSizeMs, higher);
}

TEST_F(DelayManagerTest, UpdateLongInterArrivalTime) {
  SetPacketAudioLength(kFrameSizeMs);
  
  InsertNextPacket();
  
  IncreaseTime(2 * kFrameSizeMs);
  
  
  
  
  EXPECT_CALL(detector_, Update(2, 2))
      .WillOnce(Return(false));
  InsertNextPacket();
  EXPECT_EQ(2 << 8, dm_->TargetLevel());  
  EXPECT_EQ(2, dm_->base_target_level());
  int lower, higher;
  dm_->BufferLimits(&lower, &higher);
  
  
  
  EXPECT_EQ((2 << 8) * 3 / 4, lower);
  EXPECT_EQ(lower + (20 << 8) / kFrameSizeMs, higher);
}

TEST_F(DelayManagerTest, UpdatePeakFound) {
  SetPacketAudioLength(kFrameSizeMs);
  
  InsertNextPacket();
  
  IncreaseTime(kFrameSizeMs);
  
  
  
  
  EXPECT_CALL(detector_, Update(1, 1))
      .WillOnce(Return(true));
  EXPECT_CALL(detector_, MaxPeakHeight())
      .WillOnce(Return(5));
  InsertNextPacket();
  EXPECT_EQ(5 << 8, dm_->TargetLevel());
  EXPECT_EQ(1, dm_->base_target_level());  
  int lower, higher;
  dm_->BufferLimits(&lower, &higher);
  
  EXPECT_EQ((5 << 8) * 3 / 4, lower);
  EXPECT_EQ(5 << 8, higher);
}

TEST_F(DelayManagerTest, TargetDelay) {
  SetPacketAudioLength(kFrameSizeMs);
  
  InsertNextPacket();
  
  IncreaseTime(kFrameSizeMs);
  
  
  
  
  EXPECT_CALL(detector_, Update(1, 1))
      .WillOnce(Return(false));
  InsertNextPacket();
  const int kExpectedTarget = 1;
  EXPECT_EQ(kExpectedTarget << 8, dm_->TargetLevel());  
  EXPECT_EQ(1, dm_->base_target_level());
  int lower, higher;
  dm_->BufferLimits(&lower, &higher);
  
  
  EXPECT_EQ((1 << 8) * 3 / 4, lower);
  EXPECT_EQ(lower + (20 << 8) / kFrameSizeMs, higher);
}

TEST_F(DelayManagerTest, MaxAndRequiredDelay) {
  const int kExpectedTarget = 5;
  const int kTimeIncrement = kExpectedTarget * kFrameSizeMs;
  SetPacketAudioLength(kFrameSizeMs);
  
  InsertNextPacket();
  
  
  
  EXPECT_CALL(detector_, Update(kExpectedTarget, _))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(detector_, MaxPeakHeight())
      .WillRepeatedly(Return(kExpectedTarget));
  IncreaseTime(kTimeIncrement);
  InsertNextPacket();

  
  EXPECT_EQ(kExpectedTarget << 8, dm_->TargetLevel());

  int kMaxDelayPackets = kExpectedTarget - 2;
  int kMaxDelayMs = kMaxDelayPackets * kFrameSizeMs;
  EXPECT_TRUE(dm_->SetMaximumDelay(kMaxDelayMs));
  IncreaseTime(kTimeIncrement);
  InsertNextPacket();
  EXPECT_EQ(kExpectedTarget * kFrameSizeMs, dm_->least_required_delay_ms());
  EXPECT_EQ(kMaxDelayPackets << 8, dm_->TargetLevel());

  
  EXPECT_FALSE(dm_->SetMaximumDelay(kFrameSizeMs - 1));
}

TEST_F(DelayManagerTest, MinAndRequiredDelay) {
  const int kExpectedTarget = 5;
  const int kTimeIncrement = kExpectedTarget * kFrameSizeMs;
  SetPacketAudioLength(kFrameSizeMs);
  
  InsertNextPacket();
  
  
  
  EXPECT_CALL(detector_, Update(kExpectedTarget, _))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(detector_, MaxPeakHeight())
      .WillRepeatedly(Return(kExpectedTarget));
  IncreaseTime(kTimeIncrement);
  InsertNextPacket();

  
  EXPECT_EQ(kExpectedTarget << 8, dm_->TargetLevel());

  int kMinDelayPackets = kExpectedTarget + 2;
  int kMinDelayMs = kMinDelayPackets * kFrameSizeMs;
  dm_->SetMinimumDelay(kMinDelayMs);
  IncreaseTime(kTimeIncrement);
  InsertNextPacket();
  EXPECT_EQ(kExpectedTarget * kFrameSizeMs, dm_->least_required_delay_ms());
  EXPECT_EQ(kMinDelayPackets << 8, dm_->TargetLevel());
}

TEST_F(DelayManagerTest, Failures) {
  
  EXPECT_EQ(-1, dm_->Update(0, 0, -1));
  
  EXPECT_EQ(-1, dm_->SetPacketAudioLength(0));
  EXPECT_EQ(-1, dm_->SetPacketAudioLength(-1));

  
  EXPECT_TRUE(dm_->SetMaximumDelay(10));
  EXPECT_FALSE(dm_->SetMinimumDelay(20));

  
  EXPECT_TRUE(dm_->SetMaximumDelay(100));
  EXPECT_TRUE(dm_->SetMinimumDelay(80));
  EXPECT_FALSE(dm_->SetMaximumDelay(60));
}

}  
