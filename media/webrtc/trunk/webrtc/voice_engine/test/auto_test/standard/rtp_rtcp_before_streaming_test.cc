









#include "after_initialization_fixture.h"

using namespace webrtc;
using namespace testing;

class RtpRtcpBeforeStreamingTest : public AfterInitializationFixture {
 protected:
  void SetUp();
  void TearDown();

  int channel_;
};

void RtpRtcpBeforeStreamingTest::SetUp() {
  EXPECT_THAT(channel_ = voe_base_->CreateChannel(), Not(Lt(0)));
}

void RtpRtcpBeforeStreamingTest::TearDown() {
  EXPECT_EQ(0, voe_base_->DeleteChannel(channel_));
}

TEST_F(RtpRtcpBeforeStreamingTest,
       GetRtcpStatusReturnsTrueByDefaultAndObeysSetRtcpStatus) {
  bool on = false;
  EXPECT_EQ(0, voe_rtp_rtcp_->GetRTCPStatus(channel_, on));
  EXPECT_TRUE(on);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetRTCPStatus(channel_, false));
  EXPECT_EQ(0, voe_rtp_rtcp_->GetRTCPStatus(channel_, on));
  EXPECT_FALSE(on);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetRTCPStatus(channel_, true));
  EXPECT_EQ(0, voe_rtp_rtcp_->GetRTCPStatus(channel_, on));
  EXPECT_TRUE(on);
}

TEST_F(RtpRtcpBeforeStreamingTest, GetLocalSsrcObeysSetLocalSsrc) {
  EXPECT_EQ(0, voe_rtp_rtcp_->SetLocalSSRC(channel_, 1234));
  unsigned int result = 0;
  EXPECT_EQ(0, voe_rtp_rtcp_->GetLocalSSRC(channel_, result));
  EXPECT_EQ(1234u, result);
}

TEST_F(RtpRtcpBeforeStreamingTest, GetLastRemoteTimeStamp) {
  uint32_t timestamp;
  EXPECT_EQ(0, voe_rtp_rtcp_->GetLastRemoteTimeStamp(channel_, &timestamp));
  EXPECT_EQ(0u, timestamp);
}
