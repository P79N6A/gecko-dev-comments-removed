









#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "webrtc/modules/pacing/include/paced_sender.h"

using testing::_;

namespace webrtc {
namespace test {

static const int kTargetBitrate = 800;
static const float kPaceMultiplier = 1.5f;

class MockPacedSenderCallback : public PacedSender::Callback {
 public:
  MOCK_METHOD3(TimeToSendPacket,
      void(uint32_t ssrc, uint16_t sequence_number, int64_t capture_time_ms));
  MOCK_METHOD1(TimeToSendPadding,
      void(int bytes));
};

class PacedSenderTest : public ::testing::Test {
 protected:
  PacedSenderTest() {
    TickTime::UseFakeClock(123456);
    
    send_bucket_.reset(new PacedSender(&callback_, kTargetBitrate,
                                       kPaceMultiplier));
    send_bucket_->SetStatus(true);
  }
  MockPacedSenderCallback callback_;
  scoped_ptr<PacedSender> send_bucket_;
};

TEST_F(PacedSenderTest, QueuePacket) {
  uint32_t ssrc = 12345;
  uint16_t sequence_number = 1234;
  int64_t capture_time_ms = 56789;

  
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number, capture_time_ms, 250));
  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  EXPECT_CALL(callback_, TimeToSendPadding(_)).Times(0);
  EXPECT_CALL(callback_,
      TimeToSendPacket(ssrc, sequence_number, capture_time_ms)).Times(0);
  TickTime::AdvanceFakeClock(4);
  EXPECT_EQ(1, send_bucket_->TimeUntilNextProcess());
  EXPECT_CALL(callback_,
      TimeToSendPacket(ssrc, sequence_number, capture_time_ms)).Times(1);
  TickTime::AdvanceFakeClock(1);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());
  sequence_number++;
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
}

TEST_F(PacedSenderTest, PaceQueuedPackets) {
  uint32_t ssrc = 12345;
  uint16_t sequence_number = 1234;
  int64_t capture_time_ms = 56789;

  
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
        sequence_number++, capture_time_ms, 250));
  }
  for (int j = 0; j < 30; ++j) {
    EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
        sequence_number++, capture_time_ms, 250));
  }
  EXPECT_CALL(callback_, TimeToSendPadding(_)).Times(0);
  for (int k = 0; k < 10; ++k) {
    EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
    TickTime::AdvanceFakeClock(5);
    EXPECT_CALL(callback_,
        TimeToSendPacket(ssrc, _, capture_time_ms)).Times(3);
    EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
    EXPECT_EQ(0, send_bucket_->Process());
  }
  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  TickTime::AdvanceFakeClock(5);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
}

TEST_F(PacedSenderTest, PaceQueuedPacketsWithDuplicates) {
  uint32_t ssrc = 12345;
  uint16_t sequence_number = 1234;
  int64_t capture_time_ms = 56789;
  uint16_t queued_sequence_number;

  
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
        sequence_number++, capture_time_ms, 250));
  }
  queued_sequence_number = sequence_number;

  for (int j = 0; j < 30; ++j) {
    
    EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
        sequence_number, capture_time_ms, 250));
    EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
        sequence_number++, capture_time_ms, 250));
  }
  EXPECT_CALL(callback_, TimeToSendPadding(_)).Times(0);
  for (int k = 0; k < 10; ++k) {
    EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
    TickTime::AdvanceFakeClock(5);

    for (int i = 0; i < 3; ++i) {
      EXPECT_CALL(callback_, TimeToSendPacket(ssrc, queued_sequence_number++,
                                              capture_time_ms)).Times(1);
   }
    EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
    EXPECT_EQ(0, send_bucket_->Process());
  }
  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  TickTime::AdvanceFakeClock(5);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
}

TEST_F(PacedSenderTest, Padding) {
  uint32_t ssrc = 12345;
  uint16_t sequence_number = 1234;
  int64_t capture_time_ms = 56789;

  
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority, ssrc,
      sequence_number++, capture_time_ms, 250));
  EXPECT_CALL(callback_, TimeToSendPadding(250)).Times(1);
  EXPECT_CALL(callback_,
      TimeToSendPacket(ssrc, sequence_number, capture_time_ms)).Times(0);
  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  TickTime::AdvanceFakeClock(5);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());

  EXPECT_CALL(callback_, TimeToSendPadding(500)).Times(1);
  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  TickTime::AdvanceFakeClock(5);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());
}

TEST_F(PacedSenderTest, Priority) {
  uint32_t ssrc_low_priority = 12345;
  uint32_t ssrc = 12346;
  uint16_t sequence_number = 1234;
  int64_t capture_time_ms = 56789;
  int64_t capture_time_ms_low_priority = 1234567;

  
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kLowPriority,
      ssrc_low_priority, sequence_number++, capture_time_ms_low_priority, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority,
      ssrc, sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority,
      ssrc, sequence_number++, capture_time_ms, 250));

  
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kLowPriority,
      ssrc_low_priority, sequence_number++, capture_time_ms_low_priority, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority,
      ssrc, sequence_number++, capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority,
      ssrc, sequence_number++, capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kHighPriority,
      ssrc, sequence_number++, capture_time_ms, 250));

  
  EXPECT_CALL(callback_, TimeToSendPadding(_)).Times(0);
  EXPECT_CALL(callback_, TimeToSendPacket(ssrc, _, capture_time_ms)).Times(3);

  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  TickTime::AdvanceFakeClock(5);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());

  EXPECT_CALL(callback_, TimeToSendPacket(
      ssrc_low_priority, _, capture_time_ms_low_priority)).Times(1);

  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  TickTime::AdvanceFakeClock(5);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());
}

TEST_F(PacedSenderTest, Pause) {
  uint32_t ssrc_low_priority = 12345;
  uint32_t ssrc = 12346;
  uint16_t sequence_number = 1234;
  int64_t capture_time_ms = TickTime::MillisecondTimestamp();
  TickTime::AdvanceFakeClock(10000);
  int64_t second_capture_time_ms = TickTime::MillisecondTimestamp();

  EXPECT_EQ(0, send_bucket_->QueueInMs());

  
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kLowPriority,
      ssrc_low_priority, sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority,
      ssrc, sequence_number++, capture_time_ms, 250));
  EXPECT_TRUE(send_bucket_->SendPacket(PacedSender::kNormalPriority,
      ssrc, sequence_number++, capture_time_ms, 250));

  send_bucket_->Pause();

  
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kLowPriority,
      ssrc_low_priority, sequence_number++, capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority,
      ssrc, sequence_number++, capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kNormalPriority,
      ssrc, sequence_number++, second_capture_time_ms, 250));
  EXPECT_FALSE(send_bucket_->SendPacket(PacedSender::kHighPriority,
      ssrc, sequence_number++, capture_time_ms, 250));

  EXPECT_EQ(TickTime::MillisecondTimestamp() - capture_time_ms,
            send_bucket_->QueueInMs());

  
  EXPECT_CALL(callback_, TimeToSendPadding(_)).Times(0);
  EXPECT_CALL(callback_, TimeToSendPacket(_, _, _)).Times(0);

  for (int i = 0; i < 10; ++i) {
    TickTime::AdvanceFakeClock(5);
    EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
    EXPECT_EQ(0, send_bucket_->Process());
  }
  
  
  EXPECT_CALL(callback_, TimeToSendPacket(_, _, capture_time_ms)).Times(3);

  send_bucket_->Resume();

  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  TickTime::AdvanceFakeClock(5);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());

  EXPECT_CALL(callback_,
              TimeToSendPacket(_, _, second_capture_time_ms)).Times(1);

  EXPECT_EQ(5, send_bucket_->TimeUntilNextProcess());
  TickTime::AdvanceFakeClock(5);
  EXPECT_EQ(0, send_bucket_->TimeUntilNextProcess());
  EXPECT_EQ(0, send_bucket_->Process());
  EXPECT_EQ(0, send_bucket_->QueueInMs());
}

}  
}  
