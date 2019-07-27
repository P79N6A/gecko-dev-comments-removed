













#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_vp8.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_vp8_test_helper.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/typedefs.h"

#define CHECK_ARRAY_SIZE(expected_size, array)                      \
  COMPILE_ASSERT(expected_size == sizeof(array) / sizeof(array[0]), \
                 check_array_size);

namespace webrtc {
namespace {































void VerifyBasicHeader(RTPTypeHeader* type, bool N, bool S, int part_id) {
  ASSERT_TRUE(type != NULL);
  EXPECT_EQ(N, type->Video.codecHeader.VP8.nonReference);
  EXPECT_EQ(S, type->Video.codecHeader.VP8.beginningOfPartition);
  EXPECT_EQ(part_id, type->Video.codecHeader.VP8.partitionId);
}

void VerifyExtensions(RTPTypeHeader* type,
                      int16_t picture_id,   
                      int16_t tl0_pic_idx,  
                      uint8_t temporal_idx, 
                      int key_idx ) {
  ASSERT_TRUE(type != NULL);
  EXPECT_EQ(picture_id, type->Video.codecHeader.VP8.pictureId);
  EXPECT_EQ(tl0_pic_idx, type->Video.codecHeader.VP8.tl0PicIdx);
  EXPECT_EQ(temporal_idx, type->Video.codecHeader.VP8.temporalIdx);
  EXPECT_EQ(key_idx, type->Video.codecHeader.VP8.keyIdx);
}
}  

class RtpPacketizerVp8Test : public ::testing::Test {
 protected:
  RtpPacketizerVp8Test() : helper_(NULL) {}
  virtual void TearDown() { delete helper_; }
  bool Init(const int* partition_sizes, int num_partitions) {
    hdr_info_.pictureId = kNoPictureId;
    hdr_info_.nonReference = false;
    hdr_info_.temporalIdx = kNoTemporalIdx;
    hdr_info_.layerSync = false;
    hdr_info_.tl0PicIdx = kNoTl0PicIdx;
    hdr_info_.keyIdx = kNoKeyIdx;
    if (helper_ != NULL)
      return false;
    helper_ = new test::RtpFormatVp8TestHelper(&hdr_info_);
    return helper_->Init(partition_sizes, num_partitions);
  }

  RTPVideoHeaderVP8 hdr_info_;
  test::RtpFormatVp8TestHelper* helper_;
};

TEST_F(RtpPacketizerVp8Test, TestStrictMode) {
  const int kSizeVector[] = {10, 8, 27};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 200;  
  const int kMaxSize = 13;
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize, kStrict);
  packetizer.SetPayloadData(helper_->payload_data(),
                            helper_->payload_size(),
                            helper_->fragmentation());

  
  const int kExpectedSizes[] = {9, 9, 12, 11, 11, 11, 10};
  const int kExpectedPart[] = {0, 0, 1, 2, 2, 2, 2};
  const bool kExpectedFragStart[] = {true,  false, true, true,
                                     false, false, false};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}

TEST_F(RtpPacketizerVp8Test, TestAggregateMode) {
  const int kSizeVector[] = {60, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 20;  
  const int kMaxSize = 25;
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize, kAggregate);
  packetizer.SetPayloadData(helper_->payload_data(),
                            helper_->payload_size(),
                            helper_->fragmentation());

  
  const int kExpectedSizes[] = {23, 23, 23, 23};
  const int kExpectedPart[] = {0, 0, 0, 1};
  const bool kExpectedFragStart[] = {true, false, false, true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}

TEST_F(RtpPacketizerVp8Test, TestAggregateModeManyPartitions1) {
  const int kSizeVector[] = {1600, 200, 200, 200, 200, 200, 200, 200, 200};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 20;  
  const int kMaxSize = 1500;
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize, kAggregate);
  packetizer.SetPayloadData(helper_->payload_data(),
                            helper_->payload_size(),
                            helper_->fragmentation());

  
  const int kExpectedSizes[] = {803, 803, 803, 803};
  const int kExpectedPart[] = {0, 0, 1, 5};
  const bool kExpectedFragStart[] = {true, false, true, true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}

TEST_F(RtpPacketizerVp8Test, TestAggregateModeManyPartitions2) {
  const int kSizeVector[] = {1599, 200, 200, 200, 1600, 200, 200, 200, 200};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 20;  
  const int kMaxSize = 1500;
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize, kAggregate);
  packetizer.SetPayloadData(helper_->payload_data(),
                            helper_->payload_size(),
                            helper_->fragmentation());

  
  const int kExpectedSizes[] = {803, 802, 603, 803, 803, 803};
  const int kExpectedPart[] = {0, 0, 1, 4, 4, 5};
  const bool kExpectedFragStart[] = {true, false, true, true, false, true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}

TEST_F(RtpPacketizerVp8Test, TestAggregateModeTwoLargePartitions) {
  const int kSizeVector[] = {1654, 2268};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 20;  
  const int kMaxSize = 1460;
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize, kAggregate);
  packetizer.SetPayloadData(helper_->payload_data(),
                            helper_->payload_size(),
                            helper_->fragmentation());

  
  const int kExpectedSizes[] = {830, 830, 1137, 1137};
  const int kExpectedPart[] = {0, 0, 1, 1};
  const bool kExpectedFragStart[] = {true, false, true, false};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}


TEST_F(RtpPacketizerVp8Test, TestEqualSizeModeFallback) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 200;  
  const int kMaxSize = 12;    
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize);
  packetizer.SetPayloadData(
      helper_->payload_data(), helper_->payload_size(), NULL);

  
  const int kExpectedSizes[] = {12, 11, 12, 11};
  const int kExpectedPart[] = {0, 0, 0, 0};  
  
  const bool kExpectedFragStart[] = {true, false, false, false};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->set_sloppy_partitioning(true);
  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}


TEST_F(RtpPacketizerVp8Test, TestNonReferenceBit) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.nonReference = true;
  const int kMaxSize = 25;  
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize);
  packetizer.SetPayloadData(
      helper_->payload_data(), helper_->payload_size(), NULL);

  
  const int kExpectedSizes[] = {16, 16};
  const int kExpectedPart[] = {0, 0};  
  
  const bool kExpectedFragStart[] = {true, false};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->set_sloppy_partitioning(true);
  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}


TEST_F(RtpPacketizerVp8Test, TestTl0PicIdxAndTID) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.tl0PicIdx = 117;
  hdr_info_.temporalIdx = 2;
  hdr_info_.layerSync = true;
  
  const int kMaxSize = helper_->buffer_size();
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize, kAggregate);
  packetizer.SetPayloadData(helper_->payload_data(),
                            helper_->payload_size(),
                            helper_->fragmentation());

  
  const int kExpectedSizes[1] = {helper_->payload_size() + 4};
  const int kExpectedPart[1] = {0};  
  const bool kExpectedFragStart[1] = {true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}


TEST_F(RtpPacketizerVp8Test, TestKeyIdx) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.keyIdx = 17;
  
  const int kMaxSize = helper_->buffer_size();
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize, kAggregate);
  packetizer.SetPayloadData(helper_->payload_data(),
                            helper_->payload_size(),
                            helper_->fragmentation());

  
  const int kExpectedSizes[1] = {helper_->payload_size() + 3};
  const int kExpectedPart[1] = {0};  
  const bool kExpectedFragStart[1] = {true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}


TEST_F(RtpPacketizerVp8Test, TestTIDAndKeyIdx) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.temporalIdx = 1;
  hdr_info_.keyIdx = 5;
  
  const int kMaxSize = helper_->buffer_size();
  RtpPacketizerVp8 packetizer(hdr_info_, kMaxSize, kAggregate);
  packetizer.SetPayloadData(helper_->payload_data(),
                            helper_->payload_size(),
                            helper_->fragmentation());

  
  const int kExpectedSizes[1] = {helper_->payload_size() + 3};
  const int kExpectedPart[1] = {0};  
  const bool kExpectedFragStart[1] = {true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedPart);
  CHECK_ARRAY_SIZE(kExpectedNum, kExpectedFragStart);

  helper_->GetAllPacketsAndCheck(&packetizer,
                                 kExpectedSizes,
                                 kExpectedPart,
                                 kExpectedFragStart,
                                 kExpectedNum);
}

class RtpDepacketizerVp8Test : public ::testing::Test {
 protected:
  RtpDepacketizerVp8Test()
      : depacketizer_(RtpDepacketizer::Create(kRtpVideoVp8)) {}

  void ExpectPacket(RtpDepacketizer::ParsedPayload* parsed_payload,
                    const uint8_t* data,
                    size_t length) {
    ASSERT_TRUE(parsed_payload != NULL);
    EXPECT_THAT(std::vector<uint8_t>(
                    parsed_payload->payload,
                    parsed_payload->payload + parsed_payload->payload_length),
                ::testing::ElementsAreArray(data, length));
  }

  scoped_ptr<RtpDepacketizer> depacketizer_;
};

TEST_F(RtpDepacketizerVp8Test, BasicHeader) {
  const uint8_t kHeaderLength = 1;
  uint8_t packet[4] = {0};
  packet[0] = 0x14;  
  packet[1] = 0x01;  
  RtpDepacketizer::ParsedPayload payload;

  ASSERT_TRUE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
  ExpectPacket(
      &payload, packet + kHeaderLength, sizeof(packet) - kHeaderLength);
  EXPECT_EQ(kVideoFrameDelta, payload.frame_type);
  EXPECT_EQ(kRtpVideoVp8, payload.type.Video.codec);
  VerifyBasicHeader(&payload.type, 0, 1, 4);
  VerifyExtensions(
      &payload.type, kNoPictureId, kNoTl0PicIdx, kNoTemporalIdx, kNoKeyIdx);
}

TEST_F(RtpDepacketizerVp8Test, PictureID) {
  const uint8_t kHeaderLength1 = 3;
  const uint8_t kHeaderLength2 = 4;
  const uint8_t kPictureId = 17;
  uint8_t packet[10] = {0};
  packet[0] = 0xA0;
  packet[1] = 0x80;
  packet[2] = kPictureId;
  RtpDepacketizer::ParsedPayload payload;

  ASSERT_TRUE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
  ExpectPacket(
      &payload, packet + kHeaderLength1, sizeof(packet) - kHeaderLength1);
  EXPECT_EQ(kVideoFrameDelta, payload.frame_type);
  EXPECT_EQ(kRtpVideoVp8, payload.type.Video.codec);
  VerifyBasicHeader(&payload.type, 1, 0, 0);
  VerifyExtensions(
      &payload.type, kPictureId, kNoTl0PicIdx, kNoTemporalIdx, kNoKeyIdx);

  
  packet[2] = 0x80 | kPictureId;
  packet[3] = kPictureId;

  payload = RtpDepacketizer::ParsedPayload();
  ASSERT_TRUE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
  ExpectPacket(
      &payload, packet + kHeaderLength2, sizeof(packet) - kHeaderLength2);
  VerifyBasicHeader(&payload.type, 1, 0, 0);
  VerifyExtensions(&payload.type,
                   (kPictureId << 8) + kPictureId,
                   kNoTl0PicIdx,
                   kNoTemporalIdx,
                   kNoKeyIdx);
}

TEST_F(RtpDepacketizerVp8Test, Tl0PicIdx) {
  const uint8_t kHeaderLength = 3;
  const uint8_t kTl0PicIdx = 17;
  uint8_t packet[13] = {0};
  packet[0] = 0x90;
  packet[1] = 0x40;
  packet[2] = kTl0PicIdx;
  RtpDepacketizer::ParsedPayload payload;

  ASSERT_TRUE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
  ExpectPacket(
      &payload, packet + kHeaderLength, sizeof(packet) - kHeaderLength);
  EXPECT_EQ(kVideoFrameKey, payload.frame_type);
  EXPECT_EQ(kRtpVideoVp8, payload.type.Video.codec);
  VerifyBasicHeader(&payload.type, 0, 1, 0);
  VerifyExtensions(
      &payload.type, kNoPictureId, kTl0PicIdx, kNoTemporalIdx, kNoKeyIdx);
}

TEST_F(RtpDepacketizerVp8Test, TIDAndLayerSync) {
  const uint8_t kHeaderLength = 3;
  uint8_t packet[10] = {0};
  packet[0] = 0x88;
  packet[1] = 0x20;
  packet[2] = 0x80;  
  RtpDepacketizer::ParsedPayload payload;

  ASSERT_TRUE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
  ExpectPacket(
      &payload, packet + kHeaderLength, sizeof(packet) - kHeaderLength);
  EXPECT_EQ(kVideoFrameDelta, payload.frame_type);
  EXPECT_EQ(kRtpVideoVp8, payload.type.Video.codec);
  VerifyBasicHeader(&payload.type, 0, 0, 8);
  VerifyExtensions(&payload.type, kNoPictureId, kNoTl0PicIdx, 2, kNoKeyIdx);
  EXPECT_FALSE(payload.type.Video.codecHeader.VP8.layerSync);
}

TEST_F(RtpDepacketizerVp8Test, KeyIdx) {
  const uint8_t kHeaderLength = 3;
  const uint8_t kKeyIdx = 17;
  uint8_t packet[10] = {0};
  packet[0] = 0x88;
  packet[1] = 0x10;  
  packet[2] = kKeyIdx;
  RtpDepacketizer::ParsedPayload payload;

  ASSERT_TRUE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
  ExpectPacket(
      &payload, packet + kHeaderLength, sizeof(packet) - kHeaderLength);
  EXPECT_EQ(kVideoFrameDelta, payload.frame_type);
  EXPECT_EQ(kRtpVideoVp8, payload.type.Video.codec);
  VerifyBasicHeader(&payload.type, 0, 0, 8);
  VerifyExtensions(
      &payload.type, kNoPictureId, kNoTl0PicIdx, kNoTemporalIdx, kKeyIdx);
}

TEST_F(RtpDepacketizerVp8Test, MultipleExtensions) {
  const uint8_t kHeaderLength = 6;
  uint8_t packet[10] = {0};
  packet[0] = 0x88;
  packet[1] = 0x80 | 0x40 | 0x20 | 0x10;
  packet[2] = 0x80 | 17;           
  packet[3] = 17;                  
  packet[4] = 42;                  
  packet[5] = 0x40 | 0x20 | 0x11;  
  RtpDepacketizer::ParsedPayload payload;

  ASSERT_TRUE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
  ExpectPacket(
      &payload, packet + kHeaderLength, sizeof(packet) - kHeaderLength);
  EXPECT_EQ(kVideoFrameDelta, payload.frame_type);
  EXPECT_EQ(kRtpVideoVp8, payload.type.Video.codec);
  VerifyBasicHeader(&payload.type, 0, 0, 8);
  VerifyExtensions(&payload.type, (17 << 8) + 17, 42, 1, 17);
}

TEST_F(RtpDepacketizerVp8Test, TooShortHeader) {
  uint8_t packet[4] = {0};
  packet[0] = 0x88;
  packet[1] = 0x80 | 0x40 | 0x20 | 0x10;  
  packet[2] = 0x80 | 17;  
  packet[3] = 17;         
  RtpDepacketizer::ParsedPayload payload;

  EXPECT_FALSE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
}

TEST_F(RtpDepacketizerVp8Test, TestWithPacketizer) {
  const uint8_t kHeaderLength = 5;
  uint8_t data[10] = {0};
  uint8_t packet[20] = {0};
  RTPVideoHeaderVP8 input_header;
  input_header.nonReference = true;
  input_header.pictureId = 300;
  input_header.temporalIdx = 1;
  input_header.layerSync = false;
  input_header.tl0PicIdx = kNoTl0PicIdx;  
  input_header.keyIdx = 31;
  RtpPacketizerVp8 packetizer(input_header, 20);
  packetizer.SetPayloadData(data, 10, NULL);
  bool last;
  size_t send_bytes;
  ASSERT_TRUE(packetizer.NextPacket(packet, &send_bytes, &last));
  ASSERT_TRUE(last);
  RtpDepacketizer::ParsedPayload payload;

  ASSERT_TRUE(depacketizer_->Parse(&payload, packet, sizeof(packet)));
  ExpectPacket(
      &payload, packet + kHeaderLength, sizeof(packet) - kHeaderLength);
  EXPECT_EQ(kVideoFrameKey, payload.frame_type);
  EXPECT_EQ(kRtpVideoVp8, payload.type.Video.codec);
  VerifyBasicHeader(&payload.type, 1, 1, 0);
  VerifyExtensions(&payload.type,
                   input_header.pictureId,
                   input_header.tl0PicIdx,
                   input_header.temporalIdx,
                   input_header.keyIdx);
  EXPECT_EQ(payload.type.Video.codecHeader.VP8.layerSync,
            input_header.layerSync);
}
}  
