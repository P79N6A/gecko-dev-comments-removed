









#include <vector>

#include "webrtc/modules/video_coding/codecs/interface/mock/mock_video_codec_interface.h"
#include "webrtc/modules/video_coding/main/interface/mock/mock_vcm_callbacks.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"

#include "gtest/gtest.h"

using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Field;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;

namespace webrtc {

class TestVideoCodingModule : public ::testing::Test {
 protected:
  static const int kDefaultWidth = 1280;
  static const int kDefaultHeight = 720;
  static const int kNumberOfStreams = 3;
  static const int kNumberOfLayers = 3;
  static const int kUnusedPayloadType = 10;

  virtual void SetUp() {
    TickTime::UseFakeClock(0);
    vcm_ = VideoCodingModule::Create(0);
    EXPECT_EQ(0, vcm_->InitializeReceiver());
    EXPECT_EQ(0, vcm_->InitializeSender());
    EXPECT_EQ(0, vcm_->RegisterExternalEncoder(&encoder_, kUnusedPayloadType,
                                               false));
    EXPECT_EQ(0, vcm_->RegisterExternalDecoder(&decoder_, kUnusedPayloadType,
                                               true));
    memset(&settings_, 0, sizeof(settings_));
    EXPECT_EQ(0, vcm_->Codec(kVideoCodecVP8, &settings_));
    settings_.numberOfSimulcastStreams = kNumberOfStreams;
    ConfigureStream(kDefaultWidth / 4, kDefaultHeight / 4, 100,
                    &settings_.simulcastStream[0]);
    ConfigureStream(kDefaultWidth / 2, kDefaultHeight / 2, 500,
                    &settings_.simulcastStream[1]);
    ConfigureStream(kDefaultWidth, kDefaultHeight, 1200,
                    &settings_.simulcastStream[2]);
    settings_.plType = kUnusedPayloadType;  
    EXPECT_EQ(0, vcm_->RegisterSendCodec(&settings_, 1, 1200));
    EXPECT_EQ(0, vcm_->RegisterReceiveCodec(&settings_, 1, true));
  }

  virtual void TearDown() {
    VideoCodingModule::Destroy(vcm_);
  }

  void ExpectIntraRequest(int stream) {
    if (stream == -1) {
      
      EXPECT_CALL(encoder_, Encode(
          _, _, Pointee(ElementsAre(kDeltaFrame, kDeltaFrame, kDeltaFrame))))
          .Times(1)
          .WillRepeatedly(Return(0));
      return;
    }
    assert(stream >= 0);
    assert(stream < kNumberOfStreams);
    std::vector<VideoFrameType> frame_types(kNumberOfStreams, kDeltaFrame);
    frame_types[stream] = kKeyFrame;
    EXPECT_CALL(encoder_, Encode(
        _, _, Pointee(ElementsAreArray(&frame_types[0], frame_types.size()))))
        .Times(1)
        .WillRepeatedly(Return(0));
  }

  static void ConfigureStream(int width, int height, int max_bitrate,
                              SimulcastStream* stream) {
    assert(stream);
    stream->width = width;
    stream->height = height;
    stream->maxBitrate = max_bitrate;
    stream->numberOfTemporalLayers = kNumberOfLayers;
    stream->qpMax = 45;
  }

  void InsertAndVerifyPaddingFrame(const uint8_t* payload, int length,
                                   WebRtcRTPHeader* header) {
    ASSERT_TRUE(header != NULL);
    for (int j = 0; j < 5; ++j) {
      
      EXPECT_EQ(0, vcm_->IncomingPacket(payload, 0, *header));
      ++header->header.sequenceNumber;
    }
    EXPECT_CALL(packet_request_callback_, ResendPackets(_, _))
        .Times(0);
    EXPECT_EQ(0, vcm_->Process());
    EXPECT_CALL(decoder_, Decode(_, _, _, _, _))
        .Times(0);
    EXPECT_EQ(VCM_FRAME_NOT_READY, vcm_->Decode(0));
  }

  void InsertAndVerifyDecodableFrame(const uint8_t* payload, int length,
                                     WebRtcRTPHeader* header) {
    ASSERT_TRUE(header != NULL);
    EXPECT_EQ(0, vcm_->IncomingPacket(payload, length, *header));
    ++header->header.sequenceNumber;
    EXPECT_CALL(packet_request_callback_, ResendPackets(_, _))
        .Times(0);
    EXPECT_EQ(0, vcm_->Process());
    EXPECT_CALL(decoder_, Decode(_, _, _, _, _))
        .Times(1);
    EXPECT_EQ(0, vcm_->Decode(0));
  }

  VideoCodingModule* vcm_;
  NiceMock<MockVideoDecoder> decoder_;
  NiceMock<MockVideoEncoder> encoder_;
  I420VideoFrame input_frame_;
  VideoCodec settings_;
  NiceMock<MockPacketRequestCallback> packet_request_callback_;
};

TEST_F(TestVideoCodingModule, TestIntraRequests) {
  EXPECT_EQ(0, vcm_->IntraFrameRequest(0));
  ExpectIntraRequest(0);
  EXPECT_EQ(0, vcm_->AddVideoFrame(input_frame_, NULL, NULL));
  ExpectIntraRequest(-1);
  EXPECT_EQ(0, vcm_->AddVideoFrame(input_frame_, NULL, NULL));

  EXPECT_EQ(0, vcm_->IntraFrameRequest(1));
  ExpectIntraRequest(1);
  EXPECT_EQ(0, vcm_->AddVideoFrame(input_frame_, NULL, NULL));
  ExpectIntraRequest(-1);
  EXPECT_EQ(0, vcm_->AddVideoFrame(input_frame_, NULL, NULL));

  EXPECT_EQ(0, vcm_->IntraFrameRequest(2));
  ExpectIntraRequest(2);
  EXPECT_EQ(0, vcm_->AddVideoFrame(input_frame_, NULL, NULL));
  ExpectIntraRequest(-1);
  EXPECT_EQ(0, vcm_->AddVideoFrame(input_frame_, NULL, NULL));

  EXPECT_EQ(-1, vcm_->IntraFrameRequest(3));
  ExpectIntraRequest(-1);
  EXPECT_EQ(0, vcm_->AddVideoFrame(input_frame_, NULL, NULL));

  EXPECT_EQ(-1, vcm_->IntraFrameRequest(-1));
  ExpectIntraRequest(-1);
  EXPECT_EQ(0, vcm_->AddVideoFrame(input_frame_, NULL, NULL));
}

TEST_F(TestVideoCodingModule, TestIntraRequestsInternalCapture) {
  
  EXPECT_EQ(0, vcm_->RegisterExternalEncoder(NULL, kUnusedPayloadType, false));
  
  EXPECT_EQ(0, vcm_->RegisterExternalEncoder(&encoder_, kUnusedPayloadType,
                                             true));
  EXPECT_EQ(0, vcm_->RegisterSendCodec(&settings_, 1, 1200));
  ExpectIntraRequest(0);
  EXPECT_EQ(0, vcm_->IntraFrameRequest(0));
  ExpectIntraRequest(1);
  EXPECT_EQ(0, vcm_->IntraFrameRequest(1));
  ExpectIntraRequest(2);
  EXPECT_EQ(0, vcm_->IntraFrameRequest(2));
  
  EXPECT_EQ(-1, vcm_->IntraFrameRequest(3));
  EXPECT_EQ(-1, vcm_->IntraFrameRequest(-1));
}

TEST_F(TestVideoCodingModule, PaddingOnlyFrames) {
  EXPECT_EQ(0, vcm_->SetVideoProtection(kProtectionNack, true));
  EXPECT_EQ(0, vcm_->RegisterPacketRequestCallback(&packet_request_callback_));
  const unsigned int kPaddingSize = 220;
  const uint8_t payload[kPaddingSize] = {0};
  WebRtcRTPHeader header;
  memset(&header, 0, sizeof(header));
  header.frameType = kFrameEmpty;
  header.header.markerBit = false;
  header.header.paddingLength = kPaddingSize;
  header.header.payloadType = kUnusedPayloadType;
  header.header.ssrc = 1;
  header.header.headerLength = 12;
  header.type.Video.codec = kRTPVideoVP8;
  for (int i = 0; i < 10; ++i) {
    InsertAndVerifyPaddingFrame(payload, 0, &header);
    TickTime::AdvanceFakeClock(33);
    header.header.timestamp += 3000;
  }
}

TEST_F(TestVideoCodingModule, PaddingOnlyFramesWithLosses) {
  EXPECT_EQ(0, vcm_->SetVideoProtection(kProtectionNack, true));
  EXPECT_EQ(0, vcm_->RegisterPacketRequestCallback(&packet_request_callback_));
  const unsigned int kFrameSize = 1200;
  const unsigned int kPaddingSize = 220;
  const uint8_t payload[kFrameSize] = {0};
  WebRtcRTPHeader header;
  memset(&header, 0, sizeof(header));
  header.frameType = kFrameEmpty;
  header.header.markerBit = false;
  header.header.paddingLength = kPaddingSize;
  header.header.payloadType = kUnusedPayloadType;
  header.header.ssrc = 1;
  header.header.headerLength = 12;
  header.type.Video.codec = kRTPVideoVP8;
  
  header.frameType = kVideoFrameKey;
  header.type.Video.isFirstPacket = true;
  header.header.markerBit = true;
  InsertAndVerifyDecodableFrame(payload, kFrameSize, &header);
  TickTime::AdvanceFakeClock(33);
  header.header.timestamp += 3000;

  header.frameType = kFrameEmpty;
  header.type.Video.isFirstPacket = false;
  header.header.markerBit = false;
  
  for (int i = 0; i < 10; ++i) {
    
    if (i == 3) {
      header.header.sequenceNumber += 5;
      ++i;
    }
    
    if (i == 5) {
      ++header.header.sequenceNumber;
    }
    InsertAndVerifyPaddingFrame(payload, 0, &header);
    TickTime::AdvanceFakeClock(33);
    header.header.timestamp += 3000;
  }
}

TEST_F(TestVideoCodingModule, PaddingOnlyAndVideo) {
  EXPECT_EQ(0, vcm_->SetVideoProtection(kProtectionNack, true));
  EXPECT_EQ(0, vcm_->RegisterPacketRequestCallback(&packet_request_callback_));
  const unsigned int kFrameSize = 1200;
  const unsigned int kPaddingSize = 220;
  const uint8_t payload[kFrameSize] = {0};
  WebRtcRTPHeader header;
  memset(&header, 0, sizeof(header));
  header.frameType = kFrameEmpty;
  header.type.Video.isFirstPacket = false;
  header.header.markerBit = false;
  header.header.paddingLength = kPaddingSize;
  header.header.payloadType = kUnusedPayloadType;
  header.header.ssrc = 1;
  header.header.headerLength = 12;
  header.type.Video.codec = kRTPVideoVP8;
  header.type.Video.codecHeader.VP8.pictureId = -1;
  header.type.Video.codecHeader.VP8.tl0PicIdx = -1;
  for (int i = 0; i < 3; ++i) {
    
    for (int j = 0; j < 2; ++j) {
      if (i == 0 && j == 0)  
        header.frameType = kVideoFrameKey;
      else
        header.frameType = kVideoFrameDelta;
      header.type.Video.isFirstPacket = true;
      header.header.markerBit = true;
      InsertAndVerifyDecodableFrame(payload, kFrameSize, &header);
      TickTime::AdvanceFakeClock(33);
      header.header.timestamp += 3000;
    }

    
    header.frameType = kFrameEmpty;
    header.type.Video.isFirstPacket = false;
    header.header.markerBit = false;
    for (int j = 0; j < 2; ++j) {
      InsertAndVerifyPaddingFrame(payload, 0, &header);
      TickTime::AdvanceFakeClock(33);
      header.header.timestamp += 3000;
    }
  }
}

}  
