









#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h"
#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/video_engine/include/vie_network.h"
#include "webrtc/voice_engine/test/auto_test/fixtures/before_streaming_fixture.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Eq;
using ::testing::Field;

class ExtensionVerifyTransport : public webrtc::Transport {
 public:
  ExtensionVerifyTransport()
      : parser_(webrtc::RtpHeaderParser::Create()),
        received_packets_(0),
        bad_packets_(0),
        audio_level_id_(-1),
        absolute_sender_time_id_(-1) {}

  virtual int SendPacket(int channel, const void* data, int len) OVERRIDE {
    webrtc::RTPHeader header;
    if (parser_->Parse(reinterpret_cast<const uint8_t*>(data),
                       static_cast<size_t>(len),
                       &header)) {
      bool ok = true;
      if (audio_level_id_ >= 0 &&
          !header.extension.hasAudioLevel) {
        ok = false;
      }
      if (absolute_sender_time_id_ >= 0 &&
          !header.extension.hasAbsoluteSendTime) {
        ok = false;
      }
      if (!ok) {
        
        
        ++bad_packets_;
      }
    }
    
    ++received_packets_;
    return len;
  }

  virtual int SendRTCPPacket(int channel, const void* data, int len) OVERRIDE {
    return len;
  }

  void SetAudioLevelId(int id) {
    audio_level_id_ = id;
    parser_->RegisterRtpHeaderExtension(webrtc::kRtpExtensionAudioLevel, id);
  }

  void SetAbsoluteSenderTimeId(int id) {
    absolute_sender_time_id_ = id;
    parser_->RegisterRtpHeaderExtension(webrtc::kRtpExtensionAbsoluteSendTime,
                                        id);
  }

  bool Wait() {
    
    while (received_packets_.Value() < kPacketsExpected) {
      webrtc::SleepMs(kSleepIntervalMs);
    }
    
    
    return bad_packets_.Value() == 0;
  }

 private:
  enum {
    kPacketsExpected = 10,
    kSleepIntervalMs = 10
  };
  webrtc::scoped_ptr<webrtc::RtpHeaderParser> parser_;
  webrtc::Atomic32 received_packets_;
  webrtc::Atomic32 bad_packets_;
  int audio_level_id_;
  int absolute_sender_time_id_;
};

class SendRtpRtcpHeaderExtensionsTest : public BeforeStreamingFixture {
 protected:
  virtual void SetUp() OVERRIDE {
    EXPECT_EQ(0, voe_network_->DeRegisterExternalTransport(channel_));
    EXPECT_EQ(0, voe_network_->RegisterExternalTransport(channel_,
                                                         verifying_transport_));
  }
  virtual void TearDown() OVERRIDE {
    PausePlaying();
  }

  ExtensionVerifyTransport verifying_transport_;
};

TEST_F(SendRtpRtcpHeaderExtensionsTest, SentPacketsIncludeNoAudioLevel) {
  verifying_transport_.SetAudioLevelId(0);
  ResumePlaying();
  EXPECT_FALSE(verifying_transport_.Wait());
}

TEST_F(SendRtpRtcpHeaderExtensionsTest, SentPacketsIncludeAudioLevel) {
  EXPECT_EQ(0, voe_rtp_rtcp_->SetSendAudioLevelIndicationStatus(channel_, true,
                                                                9));
  verifying_transport_.SetAudioLevelId(9);
  ResumePlaying();
  EXPECT_TRUE(verifying_transport_.Wait());
}

TEST_F(SendRtpRtcpHeaderExtensionsTest, SentPacketsIncludeNoAbsoluteSenderTime)
{
  verifying_transport_.SetAbsoluteSenderTimeId(0);
  ResumePlaying();
  EXPECT_FALSE(verifying_transport_.Wait());
}

TEST_F(SendRtpRtcpHeaderExtensionsTest, SentPacketsIncludeAbsoluteSenderTime) {
  EXPECT_EQ(0, voe_rtp_rtcp_->SetSendAbsoluteSenderTimeStatus(channel_, true,
                                                              11));
  verifying_transport_.SetAbsoluteSenderTimeId(11);
  ResumePlaying();
  EXPECT_TRUE(verifying_transport_.Wait());
}

TEST_F(SendRtpRtcpHeaderExtensionsTest, SentPacketsIncludeAllExtensions1) {
  EXPECT_EQ(0, voe_rtp_rtcp_->SetSendAudioLevelIndicationStatus(channel_, true,
                                                                9));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetSendAbsoluteSenderTimeStatus(channel_, true,
                                                              11));
  verifying_transport_.SetAudioLevelId(9);
  verifying_transport_.SetAbsoluteSenderTimeId(11);
  ResumePlaying();
  EXPECT_TRUE(verifying_transport_.Wait());
}

TEST_F(SendRtpRtcpHeaderExtensionsTest, SentPacketsIncludeAllExtensions2) {
  EXPECT_EQ(0, voe_rtp_rtcp_->SetSendAbsoluteSenderTimeStatus(channel_, true,
                                                              3));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetSendAudioLevelIndicationStatus(channel_, true,
                                                                9));
  verifying_transport_.SetAbsoluteSenderTimeId(3);
  
  
  ResumePlaying();
  EXPECT_TRUE(verifying_transport_.Wait());
}

class MockViENetwork : public webrtc::ViENetwork {
 public:
  MockViENetwork() {}
  virtual ~MockViENetwork() {}

  MOCK_METHOD0(Release, int());
  MOCK_METHOD2(SetNetworkTransmissionState, void(const int, const bool));
  MOCK_METHOD2(RegisterSendTransport, int(const int, webrtc::Transport&));
  MOCK_METHOD1(DeregisterSendTransport, int(const int));
  MOCK_METHOD4(ReceivedRTPPacket, int(const int, const void*, const int,
                                      const webrtc::PacketTime&));
  MOCK_METHOD3(ReceivedRTCPPacket, int(const int, const void*, const int));
  MOCK_METHOD2(SetMTU, int(int, unsigned int));
  MOCK_METHOD4(ReceivedBWEPacket, int(const int, int64_t, int,
                                      const webrtc::RTPHeader&));
};

class ReceiveRtpRtcpHeaderExtensionsTest : public BeforeStreamingFixture {
 protected:
  virtual void SetUp() OVERRIDE {
    EXPECT_EQ(0,
        voe_rtp_rtcp_->SetSendAbsoluteSenderTimeStatus(channel_, true, 11));
    EXPECT_EQ(0,
        voe_rtp_rtcp_->SetReceiveAbsoluteSenderTimeStatus(channel_, true, 11));
  }

  enum {
    kVideoChannelId1 = 667,
    kVideoChannelId2 = 668
  };
  MockViENetwork mock_network_;
};

TEST_F(ReceiveRtpRtcpHeaderExtensionsTest, ReceiveASTDisabled) {
  ResumePlaying();
  Sleep(500);
}

TEST_F(ReceiveRtpRtcpHeaderExtensionsTest, ReceiveASTFailSetTarget) {
  EXPECT_CALL(mock_network_, Release()).Times(1);
  EXPECT_EQ(-1, voe_rtp_rtcp_->SetVideoEngineBWETarget(-1, &mock_network_,
                                                      kVideoChannelId1));
  ResumePlaying();
}

TEST_F(ReceiveRtpRtcpHeaderExtensionsTest, ReceiveASTEnabled) {
  EXPECT_CALL(mock_network_, Release()).Times(1);
  EXPECT_CALL(mock_network_, ReceivedBWEPacket(kVideoChannelId1, _, _,
      Field(&webrtc::RTPHeader::extension,
      Field(&webrtc::RTPHeaderExtension::hasAbsoluteSendTime, Eq(true)))))
      .Times(AtLeast(1));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, &mock_network_,
                                                      kVideoChannelId1));
  ResumePlaying();
  Sleep(500);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, NULL, -1));
}

TEST_F(ReceiveRtpRtcpHeaderExtensionsTest, ReceiveASTEnabledBadExtensionId) {
  EXPECT_CALL(mock_network_, Release()).Times(1);
  EXPECT_CALL(mock_network_, ReceivedBWEPacket(kVideoChannelId1, _, _,
      Field(&webrtc::RTPHeader::extension,
      Field(&webrtc::RTPHeaderExtension::hasAbsoluteSendTime, Eq(false)))))
      .Times(AtLeast(1));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetReceiveAbsoluteSenderTimeStatus(channel_, true,
                                                                 1));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, &mock_network_,
                                                      kVideoChannelId1));
  ResumePlaying();
  Sleep(500);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, NULL, -1));
}

TEST_F(ReceiveRtpRtcpHeaderExtensionsTest, ReceiveASTEnabledNotSending) {
  EXPECT_CALL(mock_network_, Release()).Times(1);
  EXPECT_CALL(mock_network_, ReceivedBWEPacket(kVideoChannelId1, _, _,
      Field(&webrtc::RTPHeader::extension,
      Field(&webrtc::RTPHeaderExtension::hasAbsoluteSendTime, Eq(false)))))
      .Times(AtLeast(1));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetSendAbsoluteSenderTimeStatus(channel_, false,
                                                              11));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, &mock_network_,
                                                      kVideoChannelId1));
  ResumePlaying();
  Sleep(500);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, NULL, -1));
}

TEST_F(ReceiveRtpRtcpHeaderExtensionsTest, ReceiveASTEnabledNotReceiving) {
  EXPECT_CALL(mock_network_, Release()).Times(1);
  EXPECT_CALL(mock_network_, ReceivedBWEPacket(kVideoChannelId1, _, _,
      Field(&webrtc::RTPHeader::extension,
      Field(&webrtc::RTPHeaderExtension::hasAbsoluteSendTime, Eq(false)))))
      .Times(AtLeast(1));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetReceiveAbsoluteSenderTimeStatus(channel_,
                                                                 false, 11));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, &mock_network_,
                                                      kVideoChannelId1));
  ResumePlaying();
  Sleep(500);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, NULL, -1));
}

TEST_F(ReceiveRtpRtcpHeaderExtensionsTest, ReceiveASTSwitchViENetwork) {
  MockViENetwork mock_network_2;
  EXPECT_CALL(mock_network_2, Release()).Times(1);
  EXPECT_CALL(mock_network_2, ReceivedBWEPacket(kVideoChannelId1, _, _,
      Field(&webrtc::RTPHeader::extension,
      Field(&webrtc::RTPHeaderExtension::hasAbsoluteSendTime, Eq(true)))))
      .Times(AtLeast(1));
  EXPECT_CALL(mock_network_, Release()).Times(1);
  EXPECT_CALL(mock_network_, ReceivedBWEPacket(kVideoChannelId1, _, _,
      Field(&webrtc::RTPHeader::extension,
      Field(&webrtc::RTPHeaderExtension::hasAbsoluteSendTime, Eq(true)))))
      .Times(AtLeast(1));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, &mock_network_2,
                                                      kVideoChannelId1));
  ResumePlaying();
  Sleep(500);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, &mock_network_,
                                                      kVideoChannelId1));
  Sleep(500);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, NULL, -1));
}

TEST_F(ReceiveRtpRtcpHeaderExtensionsTest, ReceiveASTSwitchVideoChannel) {
  EXPECT_CALL(mock_network_, Release()).Times(2);
  EXPECT_CALL(mock_network_, ReceivedBWEPacket(kVideoChannelId1, _, _,
      Field(&webrtc::RTPHeader::extension,
      Field(&webrtc::RTPHeaderExtension::hasAbsoluteSendTime, Eq(true)))))
      .Times(AtLeast(1));
  EXPECT_CALL(mock_network_, ReceivedBWEPacket(kVideoChannelId2, _, _,
      Field(&webrtc::RTPHeader::extension,
      Field(&webrtc::RTPHeaderExtension::hasAbsoluteSendTime, Eq(true)))))
      .Times(AtLeast(1));
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, &mock_network_,
                                                      kVideoChannelId1));
  ResumePlaying();
  Sleep(500);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, &mock_network_,
                                                      kVideoChannelId2));
  Sleep(500);
  EXPECT_EQ(0, voe_rtp_rtcp_->SetVideoEngineBWETarget(channel_, NULL, -1));
}
