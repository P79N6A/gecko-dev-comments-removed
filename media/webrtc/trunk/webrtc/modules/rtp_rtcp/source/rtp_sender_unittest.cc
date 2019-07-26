













#include <gtest/gtest.h>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_header_extension.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_sender.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

namespace {
const int kId = 1;
const int kTypeLength = TRANSMISSION_TIME_OFFSET_LENGTH_IN_BYTES;
const int kPayload = 100;
const uint32_t kTimestamp = 10;
const uint16_t kSeqNum = 33;
const int kTimeOffset = 22222;
const int kMaxPacketLength = 1500;
}  

class FakeClockTest : public RtpRtcpClock {
 public:
  FakeClockTest() {
    time_in_ms_ = 123456;
  }
  
  
  virtual WebRtc_Word64 GetTimeInMS() {
    return time_in_ms_;
  }
  
  virtual void CurrentNTP(WebRtc_UWord32& secs, WebRtc_UWord32& frac) {
    secs = time_in_ms_ / 1000;
    frac = (time_in_ms_ % 1000) * 4294967;
  }
  void IncrementTime(WebRtc_UWord32 time_increment_ms) {
    time_in_ms_ += time_increment_ms;
  }
 private:
  WebRtc_Word64 time_in_ms_;
};

class LoopbackTransportTest : public webrtc::Transport {
 public:
  LoopbackTransportTest()
    : packets_sent_(0),
      last_sent_packet_len_(0) {
  }
  virtual int SendPacket(int channel, const void *data, int len) {
    packets_sent_++;
    memcpy(last_sent_packet_, data, len);
    last_sent_packet_len_ = len;
    return len;
  }
  virtual int SendRTCPPacket(int channel, const void *data, int len) {
    return -1;
  }
  int packets_sent_;
  int last_sent_packet_len_;
  uint8_t last_sent_packet_[kMaxPacketLength];
};

class RtpSenderTest : public ::testing::Test {
 protected:
  RtpSenderTest()
    : fake_clock_(),
      rtp_sender_(new RTPSender(0, false, &fake_clock_, &transport_, NULL,
                                NULL)),
      kMarkerBit(true),
      kType(kRtpExtensionTransmissionTimeOffset) {
    rtp_sender_->SetSequenceNumber(kSeqNum);
  }
  FakeClockTest fake_clock_;
  scoped_ptr<RTPSender> rtp_sender_;
  LoopbackTransportTest transport_;
  const bool kMarkerBit;
  RTPExtensionType kType;
  uint8_t packet_[kMaxPacketLength];

  void VerifyRTPHeaderCommon(const WebRtcRTPHeader& rtp_header) {
    EXPECT_EQ(kMarkerBit, rtp_header.header.markerBit);
    EXPECT_EQ(kPayload, rtp_header.header.payloadType);
    EXPECT_EQ(kSeqNum, rtp_header.header.sequenceNumber);
    EXPECT_EQ(kTimestamp, rtp_header.header.timestamp);
    EXPECT_EQ(rtp_sender_->SSRC(), rtp_header.header.ssrc);
    EXPECT_EQ(0, rtp_header.header.numCSRCs);
    EXPECT_EQ(0, rtp_header.header.paddingLength);
  }
};

TEST_F(RtpSenderTest, RegisterRtpHeaderExtension) {
  EXPECT_EQ(0, rtp_sender_->RtpHeaderExtensionTotalLength());
  EXPECT_EQ(0, rtp_sender_->RegisterRtpHeaderExtension(kType, kId));
  EXPECT_EQ(RTP_ONE_BYTE_HEADER_LENGTH_IN_BYTES + kTypeLength,
            rtp_sender_->RtpHeaderExtensionTotalLength());
  EXPECT_EQ(0, rtp_sender_->DeregisterRtpHeaderExtension(kType));
  EXPECT_EQ(0, rtp_sender_->RtpHeaderExtensionTotalLength());
}

TEST_F(RtpSenderTest, BuildRTPPacket) {
  WebRtc_Word32 length = rtp_sender_->BuildRTPheader(packet_,
                                                     kPayload,
                                                     kMarkerBit,
                                                     kTimestamp);
  EXPECT_EQ(12, length);

  
  webrtc::ModuleRTPUtility::RTPHeaderParser rtp_parser(packet_, length);
  webrtc::WebRtcRTPHeader rtp_header;

  RtpHeaderExtensionMap map;
  map.Register(kType, kId);
  const bool valid_rtp_header = rtp_parser.Parse(rtp_header, &map);

  ASSERT_TRUE(valid_rtp_header);
  ASSERT_FALSE(rtp_parser.RTCP());
  VerifyRTPHeaderCommon(rtp_header);
  EXPECT_EQ(length, rtp_header.header.headerLength);
  EXPECT_EQ(0, rtp_header.extension.transmissionTimeOffset);
}

TEST_F(RtpSenderTest, BuildRTPPacketWithTransmissionOffsetExtension) {
  EXPECT_EQ(0, rtp_sender_->SetTransmissionTimeOffset(kTimeOffset));
  EXPECT_EQ(0, rtp_sender_->RegisterRtpHeaderExtension(kType, kId));

  WebRtc_Word32 length = rtp_sender_->BuildRTPheader(packet_,
                                                     kPayload,
                                                     kMarkerBit,
                                                     kTimestamp);
  EXPECT_EQ(12 + rtp_sender_->RtpHeaderExtensionTotalLength(), length);

  
  webrtc::ModuleRTPUtility::RTPHeaderParser rtp_parser(packet_, length);
  webrtc::WebRtcRTPHeader rtp_header;

  RtpHeaderExtensionMap map;
  map.Register(kType, kId);
  const bool valid_rtp_header = rtp_parser.Parse(rtp_header, &map);

  ASSERT_TRUE(valid_rtp_header);
  ASSERT_FALSE(rtp_parser.RTCP());
  VerifyRTPHeaderCommon(rtp_header);
  EXPECT_EQ(length, rtp_header.header.headerLength);
  EXPECT_EQ(kTimeOffset, rtp_header.extension.transmissionTimeOffset);

  
  webrtc::WebRtcRTPHeader rtp_header2;
  const bool valid_rtp_header2 = rtp_parser.Parse(rtp_header2, NULL);

  ASSERT_TRUE(valid_rtp_header2);
  VerifyRTPHeaderCommon(rtp_header2);
  EXPECT_EQ(length, rtp_header2.header.headerLength);
  EXPECT_EQ(0, rtp_header2.extension.transmissionTimeOffset);
}

TEST_F(RtpSenderTest, BuildRTPPacketWithNegativeTransmissionOffsetExtension) {
  const int kNegTimeOffset = -500;
  EXPECT_EQ(0, rtp_sender_->SetTransmissionTimeOffset(kNegTimeOffset));
  EXPECT_EQ(0, rtp_sender_->RegisterRtpHeaderExtension(kType, kId));

  WebRtc_Word32 length = rtp_sender_->BuildRTPheader(packet_,
                                                     kPayload,
                                                     kMarkerBit,
                                                     kTimestamp);
  EXPECT_EQ(12 + rtp_sender_->RtpHeaderExtensionTotalLength(), length);

  
  webrtc::ModuleRTPUtility::RTPHeaderParser rtp_parser(packet_, length);
  webrtc::WebRtcRTPHeader rtp_header;

  RtpHeaderExtensionMap map;
  map.Register(kType, kId);
  const bool valid_rtp_header = rtp_parser.Parse(rtp_header, &map);

  ASSERT_TRUE(valid_rtp_header);
  ASSERT_FALSE(rtp_parser.RTCP());
  VerifyRTPHeaderCommon(rtp_header);
  EXPECT_EQ(length, rtp_header.header.headerLength);
  EXPECT_EQ(kNegTimeOffset, rtp_header.extension.transmissionTimeOffset);
}

TEST_F(RtpSenderTest, NoTrafficSmoothing) {
  WebRtc_Word32 rtp_length = rtp_sender_->BuildRTPheader(packet_,
                                                         kPayload,
                                                         kMarkerBit,
                                                         kTimestamp);

  
  EXPECT_EQ(0, rtp_sender_->SendToNetwork(packet_,
                                          0,
                                          rtp_length,
                                          kTimestamp / 90,
                                          kAllowRetransmission));
  EXPECT_EQ(1, transport_.packets_sent_);
  EXPECT_EQ(rtp_length, transport_.last_sent_packet_len_);
}

TEST_F(RtpSenderTest, DISABLED_TrafficSmoothing) {
  
  rtp_sender_->SetStorePacketsStatus(true, 10);
  EXPECT_EQ(0, rtp_sender_->RegisterRtpHeaderExtension(kType, kId));
  rtp_sender_->SetTargetSendBitrate(300000);
  WebRtc_Word32 rtp_length = rtp_sender_->BuildRTPheader(packet_,
                                                         kPayload,
                                                         kMarkerBit,
                                                         kTimestamp);
  
  EXPECT_EQ(0, rtp_sender_->SendToNetwork(packet_,
                                          0,
                                          rtp_length,
                                          fake_clock_.GetTimeInMS(),
                                          kAllowRetransmission));
  EXPECT_EQ(0, transport_.packets_sent_);
  const int kStoredTimeInMs = 100;
  fake_clock_.IncrementTime(kStoredTimeInMs);
  
  EXPECT_EQ(1, transport_.packets_sent_);
  EXPECT_EQ(rtp_length, transport_.last_sent_packet_len_);
  
  webrtc::ModuleRTPUtility::RTPHeaderParser rtp_parser(
      transport_.last_sent_packet_, rtp_length);
  webrtc::WebRtcRTPHeader rtp_header;
  RtpHeaderExtensionMap map;
  map.Register(kType, kId);
  const bool valid_rtp_header = rtp_parser.Parse(rtp_header, &map);
  ASSERT_TRUE(valid_rtp_header);
  
  EXPECT_EQ(kStoredTimeInMs * 90, rtp_header.extension.transmissionTimeOffset);
}
}  
