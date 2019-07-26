









#include <cstring>

#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/main/acm2/initial_delay_manager.h"

namespace webrtc {

namespace acm2 {

namespace {

const uint8_t kAudioPayloadType = 0;
const uint8_t kCngPayloadType = 1;
const uint8_t kAvtPayloadType = 2;

const int kSamplingRateHz = 16000;
const int kInitDelayMs = 200;
const int kFrameSizeMs = 20;
const uint32_t kTimestampStep = kFrameSizeMs * kSamplingRateHz / 1000;
const int kLatePacketThreshold = 5;

void InitRtpInfo(WebRtcRTPHeader* rtp_info) {
  memset(rtp_info, 0, sizeof(*rtp_info));
  rtp_info->header.markerBit = false;
  rtp_info->header.payloadType = kAudioPayloadType;
  rtp_info->header.sequenceNumber = 1234;
  rtp_info->header.timestamp = 0xFFFFFFFD;  
  rtp_info->header.ssrc = 0x87654321;  
  rtp_info->header.numCSRCs = 0;  
  rtp_info->header.paddingLength = 0;
  rtp_info->header.headerLength = sizeof(RTPHeader);
  rtp_info->header.payload_type_frequency = kSamplingRateHz;
  rtp_info->header.extension.absoluteSendTime = 0;
  rtp_info->header.extension.transmissionTimeOffset = 0;
  rtp_info->frameType = kAudioFrameSpeech;
}

void ForwardRtpHeader(int n,
                      WebRtcRTPHeader* rtp_info,
                      uint32_t* rtp_receive_timestamp) {
  rtp_info->header.sequenceNumber += n;
  rtp_info->header.timestamp += n * kTimestampStep;
  *rtp_receive_timestamp += n * kTimestampStep;
}

void NextRtpHeader(WebRtcRTPHeader* rtp_info,
                   uint32_t* rtp_receive_timestamp) {
  ForwardRtpHeader(1, rtp_info, rtp_receive_timestamp);
}

}  

class InitialDelayManagerTest : public ::testing::Test {
 protected:
  InitialDelayManagerTest()
      : manager_(new InitialDelayManager(kInitDelayMs, kLatePacketThreshold)),
        rtp_receive_timestamp_(1111) { }  

  virtual void SetUp() {
    ASSERT_TRUE(manager_.get() != NULL);
    InitRtpInfo(&rtp_info_);
  }

  void GetNextRtpHeader(WebRtcRTPHeader* rtp_info,
                        uint32_t* rtp_receive_timestamp) const {
    memcpy(rtp_info, &rtp_info_, sizeof(*rtp_info));
    *rtp_receive_timestamp = rtp_receive_timestamp_;
    NextRtpHeader(rtp_info, rtp_receive_timestamp);
  }

  scoped_ptr<InitialDelayManager> manager_;
  WebRtcRTPHeader rtp_info_;
  uint32_t rtp_receive_timestamp_;
};

TEST_F(InitialDelayManagerTest, Init) {
  EXPECT_TRUE(manager_->buffering());
  EXPECT_FALSE(manager_->PacketBuffered());
  manager_->DisableBuffering();
  EXPECT_FALSE(manager_->buffering());
  InitialDelayManager::SyncStream sync_stream;

  
  manager_->LatePackets(0x6789ABCD, &sync_stream);  
                                                    
  EXPECT_EQ(0, sync_stream.num_sync_packets);

  
  rtp_info_.header.payloadType = kCngPayloadType;
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kCngPacket, false,
                                     kSamplingRateHz, &sync_stream);
  EXPECT_EQ(0, sync_stream.num_sync_packets);
  ForwardRtpHeader(5, &rtp_info_, &rtp_receive_timestamp_);
  rtp_info_.header.payloadType = kAvtPayloadType;
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAvtPacket, false,
                                     kSamplingRateHz, &sync_stream);
  
  EXPECT_EQ(0, sync_stream.num_sync_packets);
  manager_->LatePackets(0x45678987, &sync_stream);  
                                                    
  
  
  EXPECT_EQ(0, sync_stream.num_sync_packets);


  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);
  rtp_info_.header.payloadType = kAudioPayloadType;
  
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, true,
                                     kSamplingRateHz, &sync_stream);
  EXPECT_EQ(0, sync_stream.num_sync_packets);

  
  manager_->LatePackets(0x6789ABCD, &sync_stream);  
                                                    
  EXPECT_EQ(0, sync_stream.num_sync_packets);

  
  
  ForwardRtpHeader(5, &rtp_info_, &rtp_receive_timestamp_);
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, true,
                                     kSamplingRateHz, &sync_stream);
}

TEST_F(InitialDelayManagerTest, MissingPacket) {
  InitialDelayManager::SyncStream sync_stream;
  
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, true,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, false,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);

  
  WebRtcRTPHeader expected_rtp_info;
  uint32_t expected_receive_timestamp;
  GetNextRtpHeader(&expected_rtp_info, &expected_receive_timestamp);

  const int kNumMissingPackets = 10;
  ForwardRtpHeader(kNumMissingPackets, &rtp_info_, &rtp_receive_timestamp_);
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, false,
                                     kSamplingRateHz, &sync_stream);
  EXPECT_EQ(kNumMissingPackets - 2, sync_stream.num_sync_packets);
  EXPECT_EQ(0, memcmp(&expected_rtp_info, &sync_stream.rtp_info,
                      sizeof(expected_rtp_info)));
  EXPECT_EQ(kTimestampStep, sync_stream.timestamp_step);
  EXPECT_EQ(expected_receive_timestamp, sync_stream.receive_timestamp);
}


TEST_F(InitialDelayManagerTest, MissingPacketEstimateTimestamp) {
  InitialDelayManager::SyncStream sync_stream;
  
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, true,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);

  
  WebRtcRTPHeader expected_rtp_info;
  uint32_t expected_receive_timestamp;
  GetNextRtpHeader(&expected_rtp_info, &expected_receive_timestamp);

  const int kNumMissingPackets = 10;
  ForwardRtpHeader(kNumMissingPackets, &rtp_info_, &rtp_receive_timestamp_);
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, false,
                                     kSamplingRateHz, &sync_stream);
  EXPECT_EQ(kNumMissingPackets - 2, sync_stream.num_sync_packets);
  EXPECT_EQ(0, memcmp(&expected_rtp_info, &sync_stream.rtp_info,
                      sizeof(expected_rtp_info)));
}

TEST_F(InitialDelayManagerTest, MissingPacketWithCng) {
  InitialDelayManager::SyncStream sync_stream;

  
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, true,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);
  rtp_info_.header.payloadType = kCngPayloadType;
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kCngPacket, false,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  rtp_info_.header.payloadType = kAudioPayloadType;
  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);

  
  const uint32_t kCngTimestampStep = 5 * kTimestampStep;
  rtp_info_.header.timestamp += kCngTimestampStep;
  rtp_receive_timestamp_ += kCngTimestampStep;

  
  WebRtcRTPHeader expected_rtp_info;
  uint32_t expected_receive_timestamp;
  GetNextRtpHeader(&expected_rtp_info, &expected_receive_timestamp);

  const int kNumMissingPackets = 10;
  ForwardRtpHeader(kNumMissingPackets, &rtp_info_, &rtp_receive_timestamp_);
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, false,
                                     kSamplingRateHz, &sync_stream);
  EXPECT_EQ(kNumMissingPackets - 2, sync_stream.num_sync_packets);
  EXPECT_EQ(0, memcmp(&expected_rtp_info, &sync_stream.rtp_info,
                      sizeof(expected_rtp_info)));
  EXPECT_EQ(kTimestampStep, sync_stream.timestamp_step);
  EXPECT_EQ(expected_receive_timestamp, sync_stream.receive_timestamp);
}

TEST_F(InitialDelayManagerTest, LatePacket) {
  InitialDelayManager::SyncStream sync_stream;
  
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, true,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, false,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  const uint32_t kTimestampStep10Ms = kSamplingRateHz / 100;

  
  uint32_t timestamp_now = rtp_receive_timestamp_ + kTimestampStep10Ms;

  
  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);

  
  WebRtcRTPHeader expected_rtp_info;
  uint32_t expected_receive_timestamp;
  GetNextRtpHeader(&expected_rtp_info, &expected_receive_timestamp);

  const int kLatePacketThreshold = 5;

  int expected_num_late_packets = kLatePacketThreshold - 1;
  for (int k = 0; k < 2; ++k) {
    for (int n = 1; n < kLatePacketThreshold * kFrameSizeMs / 10; ++n) {
      manager_->LatePackets(timestamp_now, &sync_stream);
      EXPECT_EQ(0, sync_stream.num_sync_packets) <<
          "try " << k << " loop number " << n;
      timestamp_now += kTimestampStep10Ms;
    }
    manager_->LatePackets(timestamp_now, &sync_stream);

    EXPECT_EQ(expected_num_late_packets, sync_stream.num_sync_packets) <<
        "try " << k;
    EXPECT_EQ(kTimestampStep, sync_stream.timestamp_step) <<
        "try " << k;
    EXPECT_EQ(expected_receive_timestamp, sync_stream.receive_timestamp) <<
        "try " << k;
    EXPECT_EQ(0, memcmp(&expected_rtp_info, &sync_stream.rtp_info,
                        sizeof(expected_rtp_info)));

    timestamp_now += kTimestampStep10Ms;

    
    
    
    ForwardRtpHeader(sync_stream.num_sync_packets, &expected_rtp_info,
        &expected_receive_timestamp);
    expected_num_late_packets = kLatePacketThreshold;
  }

  
  
  memcpy(&rtp_info_, &expected_rtp_info, sizeof(rtp_info_));
  rtp_receive_timestamp_ = expected_receive_timestamp;

  int kNumMissingPackets = 3;  
  ForwardRtpHeader(kNumMissingPackets, &rtp_info_, &rtp_receive_timestamp_);
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, false,
                                     kSamplingRateHz, &sync_stream);

  
  
  EXPECT_EQ(kNumMissingPackets - 1, sync_stream.num_sync_packets);
  EXPECT_EQ(kTimestampStep, sync_stream.timestamp_step);
  EXPECT_EQ(expected_receive_timestamp, sync_stream.receive_timestamp);
  EXPECT_EQ(0, memcmp(&expected_rtp_info, &sync_stream.rtp_info,
                      sizeof(expected_rtp_info)));
}

TEST_F(InitialDelayManagerTest, NoLatePacketAfterCng) {
  InitialDelayManager::SyncStream sync_stream;

  
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket, true,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);
  const uint8_t kCngPayloadType = 1;  
  rtp_info_.header.payloadType = kCngPayloadType;
  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kCngPacket, false,
                                     kSamplingRateHz, &sync_stream);
  ASSERT_EQ(0, sync_stream.num_sync_packets);

  
  uint32_t timestamp_now = rtp_receive_timestamp_ + kTimestampStep * (3 +
      kLatePacketThreshold);

  manager_->LatePackets(timestamp_now, &sync_stream);
  EXPECT_EQ(0, sync_stream.num_sync_packets);
}

TEST_F(InitialDelayManagerTest, BufferingAudio) {
  InitialDelayManager::SyncStream sync_stream;

  
  for (int n = 0; n < kInitDelayMs / kFrameSizeMs; ++n) {
    manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                       InitialDelayManager::kAudioPacket,
                                       n == 0, kSamplingRateHz, &sync_stream);
    EXPECT_EQ(0, sync_stream.num_sync_packets);
    EXPECT_TRUE(manager_->buffering());
    const uint32_t expected_playout_timestamp = rtp_info_.header.timestamp -
        kInitDelayMs * kSamplingRateHz / 1000;
    EXPECT_EQ(expected_playout_timestamp, manager_->playout_timestamp());
    NextRtpHeader(&rtp_info_, &rtp_receive_timestamp_);
  }

  manager_->UpdateLastReceivedPacket(rtp_info_, rtp_receive_timestamp_,
                                     InitialDelayManager::kAudioPacket,
                                     false, kSamplingRateHz, &sync_stream);
  EXPECT_EQ(0, sync_stream.num_sync_packets);
  EXPECT_FALSE(manager_->buffering());
}

}  

}  
