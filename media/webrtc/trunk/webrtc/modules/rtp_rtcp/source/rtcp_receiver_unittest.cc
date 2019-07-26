













#include <gmock/gmock.h>
#include <gtest/gtest.h>


#include "common_types.h"
#include "modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "modules/remote_bitrate_estimator/include/mock/mock_remote_bitrate_observer.h"
#include "modules/rtp_rtcp/source/rtp_utility.h"
#include "modules/rtp_rtcp/source/rtcp_sender.h"
#include "modules/rtp_rtcp/source/rtcp_receiver.h"
#include "modules/rtp_rtcp/source/rtp_rtcp_impl.h"

namespace webrtc {

namespace {  


class PacketBuilder {
 public:
  static const int kMaxPacketSize = 1024;

  PacketBuilder()
      : pos_(0),
        pos_of_len_(0) {
  }


  void Add8(WebRtc_UWord8 byte) {
    EXPECT_LT(pos_, kMaxPacketSize - 1);
    buffer_[pos_] = byte;
    ++ pos_;
  }

  void Add16(WebRtc_UWord16 word) {
    Add8(word >> 8);
    Add8(word & 0xFF);
  }

  void Add32(WebRtc_UWord32 word) {
    Add8(word >> 24);
    Add8((word >> 16) & 0xFF);
    Add8((word >> 8) & 0xFF);
    Add8(word & 0xFF);
  }

  void Add64(WebRtc_UWord32 upper_half, WebRtc_UWord32 lower_half) {
    Add32(upper_half);
    Add32(lower_half);
  }

  
  
  
  
  void AddRtcpHeader(int payload, int format_or_count) {
    PatchLengthField();
    Add8(0x80 | (format_or_count & 0x1F));
    Add8(payload);
    pos_of_len_ = pos_;
    Add16(0xDEAD);  
  }

  void AddTmmbrBandwidth(int mantissa, int exponent, int overhead) {
    
    WebRtc_UWord32 word = 0;
    word |= (exponent << 26);
    word |= ((mantissa & 0x1FFFF) << 9);
    word |= (overhead & 0x1FF);
    Add32(word);
  }

  void AddSrPacket(WebRtc_UWord32 sender_ssrc) {
    AddRtcpHeader(200, 0);
    Add32(sender_ssrc);
    Add64(0x10203, 0x4050607);  
    Add32(0x10203);  
    Add32(0);  
    Add32(0);  
  }

  void AddRrPacket(WebRtc_UWord32 sender_ssrc, WebRtc_UWord32 rtp_ssrc,
                   WebRtc_UWord32 extended_max) {
    AddRtcpHeader(201, 1);
    Add32(sender_ssrc);
    Add32(rtp_ssrc);
    Add32(0);  
    Add32(extended_max);
    Add32(0);  
    Add32(0);  
    Add32(0);  
  }

  const WebRtc_UWord8* packet() {
    PatchLengthField();
    return buffer_;
  }

  unsigned int length() {
    return pos_;
  }
 private:
  void PatchLengthField() {
    if (pos_of_len_ > 0) {
      
      
      int this_packet_length = (pos_ - pos_of_len_ - 2);
      ASSERT_EQ(0, this_packet_length % 4)
          << "Packets must be a multiple of 32 bits long"
          << " pos " << pos_ << " pos_of_len " << pos_of_len_;
      buffer_[pos_of_len_] = this_packet_length >> 10;
      buffer_[pos_of_len_+1] = (this_packet_length >> 2) & 0xFF;
      pos_of_len_ = 0;
    }
  }

  int pos_;
  
  
  int pos_of_len_;
  WebRtc_UWord8 buffer_[kMaxPacketSize];
};




class FakeSystemClock : public RtpRtcpClock {
 public:
  FakeSystemClock()
      : time_in_ms_(1335900000) {}  

  virtual WebRtc_Word64 GetTimeInMS() {
    return time_in_ms_;
  }

  virtual void CurrentNTP(WebRtc_UWord32& secs,
                          WebRtc_UWord32& frac) {
    secs = (time_in_ms_ / 1000) + ModuleRTPUtility::NTP_JAN_1970;
    
    frac = (WebRtc_UWord32)((time_in_ms_ % 1000)
                            * ModuleRTPUtility::NTP_FRAC / 1000);
  }

  void AdvanceClock(int ms_to_advance) {
    time_in_ms_ += ms_to_advance;
  }
 private:
  WebRtc_Word64 time_in_ms_;
};



class TestTransport : public Transport,
                      public RtpData {
 public:
  explicit TestTransport()
      : rtcp_receiver_(NULL) {
  }
  void SetRTCPReceiver(RTCPReceiver* rtcp_receiver) {
    rtcp_receiver_ = rtcp_receiver;
  }
  virtual int SendPacket(int , const void* , int ) {
    ADD_FAILURE();  
    return -1;
  }

  
  virtual int SendRTCPPacket(int , const void *packet, int packet_len) {
    ADD_FAILURE();
    return 0;
  }

  virtual int OnReceivedPayloadData(const WebRtc_UWord8* payloadData,
                                    const WebRtc_UWord16 payloadSize,
                                    const WebRtcRTPHeader* rtpHeader) {
    ADD_FAILURE();
    return 0;
  }
  RTCPReceiver* rtcp_receiver_;
};

class RtcpReceiverTest : public ::testing::Test {
 protected:
  RtcpReceiverTest()
      : over_use_detector_options_(),
        remote_bitrate_observer_(),
        remote_bitrate_estimator_(
            RemoteBitrateEstimator::Create(
                &remote_bitrate_observer_,
                over_use_detector_options_,
                RemoteBitrateEstimator::kMultiStreamEstimation)) {
    
    system_clock_ = new FakeSystemClock();
    test_transport_ = new TestTransport();

    RtpRtcp::Configuration configuration;
    configuration.id = 0;
    configuration.audio = false;
    configuration.clock = system_clock_;
    configuration.outgoing_transport = test_transport_;
    configuration.remote_bitrate_estimator = remote_bitrate_estimator_.get();
    rtp_rtcp_impl_ = new ModuleRtpRtcpImpl(configuration);
    rtcp_receiver_ = new RTCPReceiver(0, system_clock_, rtp_rtcp_impl_);
    test_transport_->SetRTCPReceiver(rtcp_receiver_);
  }
  ~RtcpReceiverTest() {
    delete rtcp_receiver_;
    delete rtp_rtcp_impl_;
    delete test_transport_;
    delete system_clock_;
  }

  
  
  int InjectRtcpPacket(const WebRtc_UWord8* packet,
                        WebRtc_UWord16 packet_len) {
    RTCPUtility::RTCPParserV2 rtcpParser(packet,
                                         packet_len,
                                         true);  

    RTCPHelp::RTCPPacketInformation rtcpPacketInformation;
    int result = rtcp_receiver_->IncomingRTCPPacket(rtcpPacketInformation,
                                                    &rtcpParser);
    rtcp_packet_info_ = rtcpPacketInformation;
    return result;
  }

  OverUseDetectorOptions over_use_detector_options_;
  FakeSystemClock* system_clock_;
  ModuleRtpRtcpImpl* rtp_rtcp_impl_;
  RTCPReceiver* rtcp_receiver_;
  TestTransport* test_transport_;
  RTCPHelp::RTCPPacketInformation rtcp_packet_info_;
  MockRemoteBitrateObserver remote_bitrate_observer_;
  scoped_ptr<RemoteBitrateEstimator> remote_bitrate_estimator_;
};


TEST_F(RtcpReceiverTest, BrokenPacketIsIgnored) {
  const WebRtc_UWord8 bad_packet[] = {0, 0, 0, 0};
  EXPECT_EQ(0, InjectRtcpPacket(bad_packet, sizeof(bad_packet)));
  EXPECT_EQ(0U, rtcp_packet_info_.rtcpPacketTypeFlags);
}

TEST_F(RtcpReceiverTest, InjectSrPacket) {
  const WebRtc_UWord32 kSenderSsrc = 0x10203;
  PacketBuilder p;
  p.AddSrPacket(kSenderSsrc);
  EXPECT_EQ(0, InjectRtcpPacket(p.packet(), p.length()));
  
  
  EXPECT_EQ(kSenderSsrc, rtcp_packet_info_.remoteSSRC);
  EXPECT_EQ(0U,
            kRtcpSr & rtcp_packet_info_.rtcpPacketTypeFlags);
}

TEST_F(RtcpReceiverTest, ReceiveReportTimeout) {
  const uint32_t kSenderSsrc = 0x10203;
  const uint32_t kSourceSsrc = 0x40506;
  const int64_t kRtcpIntervalMs = 1000;

  rtcp_receiver_->SetSSRC(kSourceSsrc);

  uint32_t sequence_number = 1234;
  system_clock_->AdvanceClock(3 * kRtcpIntervalMs);

  
  EXPECT_FALSE(rtcp_receiver_->RtcpRrTimeout(kRtcpIntervalMs));
  EXPECT_FALSE(rtcp_receiver_->RtcpRrSequenceNumberTimeout(kRtcpIntervalMs));

  
  PacketBuilder p1;
  p1.AddRrPacket(kSenderSsrc, kSourceSsrc, sequence_number);
  EXPECT_EQ(0, InjectRtcpPacket(p1.packet(), p1.length()));
  system_clock_->AdvanceClock(3 * kRtcpIntervalMs - 1);
  EXPECT_FALSE(rtcp_receiver_->RtcpRrTimeout(kRtcpIntervalMs));
  EXPECT_FALSE(rtcp_receiver_->RtcpRrSequenceNumberTimeout(kRtcpIntervalMs));

  
  
  PacketBuilder p2;
  p2.AddRrPacket(kSenderSsrc, kSourceSsrc, sequence_number);
  EXPECT_EQ(0, InjectRtcpPacket(p2.packet(), p2.length()));
  system_clock_->AdvanceClock(2);
  EXPECT_FALSE(rtcp_receiver_->RtcpRrTimeout(kRtcpIntervalMs));
  EXPECT_TRUE(rtcp_receiver_->RtcpRrSequenceNumberTimeout(kRtcpIntervalMs));

  
  system_clock_->AdvanceClock(3 * kRtcpIntervalMs);
  EXPECT_TRUE(rtcp_receiver_->RtcpRrTimeout(kRtcpIntervalMs));

  
  
  EXPECT_FALSE(rtcp_receiver_->RtcpRrTimeout(kRtcpIntervalMs));
  EXPECT_FALSE(rtcp_receiver_->RtcpRrSequenceNumberTimeout(kRtcpIntervalMs));

  
  PacketBuilder p3;
  sequence_number++;
  p2.AddRrPacket(kSenderSsrc, kSourceSsrc, sequence_number);
  EXPECT_EQ(0, InjectRtcpPacket(p2.packet(), p2.length()));
  EXPECT_FALSE(rtcp_receiver_->RtcpRrTimeout(kRtcpIntervalMs));
  EXPECT_FALSE(rtcp_receiver_->RtcpRrSequenceNumberTimeout(kRtcpIntervalMs));

  
  system_clock_->AdvanceClock(2 * kRtcpIntervalMs);
  PacketBuilder p4;
  p4.AddRrPacket(kSenderSsrc, kSourceSsrc, sequence_number);
  EXPECT_EQ(0, InjectRtcpPacket(p4.packet(), p4.length()));
  system_clock_->AdvanceClock(kRtcpIntervalMs + 1);
  EXPECT_FALSE(rtcp_receiver_->RtcpRrTimeout(kRtcpIntervalMs));
  EXPECT_TRUE(rtcp_receiver_->RtcpRrSequenceNumberTimeout(kRtcpIntervalMs));
  system_clock_->AdvanceClock(2 * kRtcpIntervalMs);
  EXPECT_TRUE(rtcp_receiver_->RtcpRrTimeout(kRtcpIntervalMs));
}

TEST_F(RtcpReceiverTest, TmmbrReceivedWithNoIncomingPacket) {
  
  EXPECT_EQ(-1, rtcp_receiver_->TMMBRReceived(0, 0, NULL));
}

TEST_F(RtcpReceiverTest, TmmbrPacketAccepted) {
  const WebRtc_UWord32 kMediaFlowSsrc = 0x2040608;
  const WebRtc_UWord32 kSenderSsrc = 0x10203;
  const WebRtc_UWord32 kMediaRecipientSsrc = 0x101;
  rtcp_receiver_->SetSSRC(kMediaFlowSsrc);  

  PacketBuilder p;
  p.AddSrPacket(kSenderSsrc);
  
  p.AddRtcpHeader(205, 3);
  p.Add32(kSenderSsrc);
  p.Add32(kMediaRecipientSsrc);
  p.Add32(kMediaFlowSsrc);
  p.AddTmmbrBandwidth(30000, 0, 0);  

  EXPECT_EQ(0, InjectRtcpPacket(p.packet(), p.length()));
  EXPECT_EQ(1, rtcp_receiver_->TMMBRReceived(0, 0, NULL));
  TMMBRSet candidate_set;
  candidate_set.VerifyAndAllocateSet(1);
  EXPECT_EQ(1, rtcp_receiver_->TMMBRReceived(1, 0, &candidate_set));
  EXPECT_LT(0U, candidate_set.Tmmbr(0));
  EXPECT_EQ(kMediaRecipientSsrc, candidate_set.Ssrc(0));
}

TEST_F(RtcpReceiverTest, TmmbrPacketNotForUsIgnored) {
  const WebRtc_UWord32 kMediaFlowSsrc = 0x2040608;
  const WebRtc_UWord32 kSenderSsrc = 0x10203;
  const WebRtc_UWord32 kMediaRecipientSsrc = 0x101;
  const WebRtc_UWord32 kOtherMediaFlowSsrc = 0x9999;

  PacketBuilder p;
  p.AddSrPacket(kSenderSsrc);
  
  p.AddRtcpHeader(205, 3);
  p.Add32(kSenderSsrc);
  p.Add32(kMediaRecipientSsrc);
  p.Add32(kOtherMediaFlowSsrc);  
  p.AddTmmbrBandwidth(30000, 0, 0);

  rtcp_receiver_->SetSSRC(kMediaFlowSsrc);
  EXPECT_EQ(0, InjectRtcpPacket(p.packet(), p.length()));
  EXPECT_EQ(0, rtcp_receiver_->TMMBRReceived(0, 0, NULL));
}

TEST_F(RtcpReceiverTest, TmmbrPacketZeroRateIgnored) {
  const WebRtc_UWord32 kMediaFlowSsrc = 0x2040608;
  const WebRtc_UWord32 kSenderSsrc = 0x10203;
  const WebRtc_UWord32 kMediaRecipientSsrc = 0x101;
  rtcp_receiver_->SetSSRC(kMediaFlowSsrc);  

  PacketBuilder p;
  p.AddSrPacket(kSenderSsrc);
  
  p.AddRtcpHeader(205, 3);
  p.Add32(kSenderSsrc);
  p.Add32(kMediaRecipientSsrc);
  p.Add32(kMediaFlowSsrc);
  p.AddTmmbrBandwidth(0, 0, 0);  

  EXPECT_EQ(0, InjectRtcpPacket(p.packet(), p.length()));
  EXPECT_EQ(0, rtcp_receiver_->TMMBRReceived(0, 0, NULL));
}

TEST_F(RtcpReceiverTest, TmmbrThreeConstraintsTimeOut) {
  const WebRtc_UWord32 kMediaFlowSsrc = 0x2040608;
  const WebRtc_UWord32 kSenderSsrc = 0x10203;
  const WebRtc_UWord32 kMediaRecipientSsrc = 0x101;
  rtcp_receiver_->SetSSRC(kMediaFlowSsrc);  

  
  
  for (WebRtc_UWord32 ssrc = kMediaRecipientSsrc;
       ssrc < kMediaRecipientSsrc+3; ++ssrc) {
    PacketBuilder p;
    p.AddSrPacket(kSenderSsrc);
    
    p.AddRtcpHeader(205, 3);
    p.Add32(kSenderSsrc);
    p.Add32(ssrc);
    p.Add32(kMediaFlowSsrc);
    p.AddTmmbrBandwidth(30000, 0, 0);  

    EXPECT_EQ(0, InjectRtcpPacket(p.packet(), p.length()));
    system_clock_->AdvanceClock(5000);  
  }
  
  EXPECT_EQ(3, rtcp_receiver_->TMMBRReceived(0, 0, NULL));
  TMMBRSet candidate_set;
  candidate_set.VerifyAndAllocateSet(3);
  EXPECT_EQ(3, rtcp_receiver_->TMMBRReceived(3, 0, &candidate_set));
  EXPECT_LT(0U, candidate_set.Tmmbr(0));
  
  
  system_clock_->AdvanceClock(12000);
  
  EXPECT_EQ(3, rtcp_receiver_->TMMBRReceived(0, 0, NULL));
  
  EXPECT_EQ(1, rtcp_receiver_->TMMBRReceived(3, 0, &candidate_set));
  EXPECT_EQ(kMediaRecipientSsrc + 2, candidate_set.Ssrc(0));
}


}  

}  
