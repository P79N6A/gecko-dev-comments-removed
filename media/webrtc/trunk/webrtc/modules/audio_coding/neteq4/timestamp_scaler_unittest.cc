









#include "webrtc/modules/audio_coding/neteq4/timestamp_scaler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/mock/mock_decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/packet.h"

using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::_;

namespace webrtc {

TEST(TimestampScaler, TestNoScaling) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderPCMu;  
  static const uint8_t kRtpPayloadType = 0;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  for (uint32_t timestamp = 0xFFFFFFFF - 5; timestamp != 5; ++timestamp) {
    
    EXPECT_EQ(timestamp, scaler.ToInternal(timestamp, kRtpPayloadType));
    
    EXPECT_EQ(timestamp, scaler.ToExternal(timestamp));
  }

  EXPECT_CALL(db, Die());  
}

TEST(TimestampScaler, TestNoScalingLargeStep) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderPCMu;  
  static const uint8_t kRtpPayloadType = 0;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  static const uint32_t kStep = 160;
  uint32_t start_timestamp = 0;
  
  start_timestamp = start_timestamp - 5 * kStep;
  for (uint32_t timestamp = start_timestamp; timestamp != 5 * kStep;
      timestamp += kStep) {
    
    EXPECT_EQ(timestamp, scaler.ToInternal(timestamp, kRtpPayloadType));
    
    EXPECT_EQ(timestamp, scaler.ToExternal(timestamp));
  }

  EXPECT_CALL(db, Die());  
}

TEST(TimestampScaler, TestG722) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderG722;  
  static const uint8_t kRtpPayloadType = 17;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  uint32_t external_timestamp = 0xFFFFFFFF - 5;
  uint32_t internal_timestamp = external_timestamp;
  for (; external_timestamp != 5; ++external_timestamp) {
    
    EXPECT_EQ(internal_timestamp,
              scaler.ToInternal(external_timestamp, kRtpPayloadType));
    
    EXPECT_EQ(external_timestamp, scaler.ToExternal(internal_timestamp));
    internal_timestamp += 2;
  }

  EXPECT_CALL(db, Die());  
}

TEST(TimestampScaler, TestG722LargeStep) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderG722;  
  static const uint8_t kRtpPayloadType = 17;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  static const uint32_t kStep = 320;
  uint32_t external_timestamp = 0;
  
  external_timestamp = external_timestamp - 5 * kStep;
  uint32_t internal_timestamp = external_timestamp;
  for (; external_timestamp != 5 * kStep; external_timestamp += kStep) {
    
    EXPECT_EQ(internal_timestamp,
              scaler.ToInternal(external_timestamp, kRtpPayloadType));
    
    EXPECT_EQ(external_timestamp, scaler.ToExternal(internal_timestamp));
    
    internal_timestamp += 2 * kStep;
  }

  EXPECT_CALL(db, Die());  
}

TEST(TimestampScaler, TestG722WithCng) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info_g722, info_cng;
  info_g722.codec_type = kDecoderG722;  
  info_cng.codec_type = kDecoderCNGwb;
  static const uint8_t kRtpPayloadTypeG722 = 17;
  static const uint8_t kRtpPayloadTypeCng = 13;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadTypeG722))
      .WillRepeatedly(Return(&info_g722));
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadTypeCng))
      .WillRepeatedly(Return(&info_cng));

  TimestampScaler scaler(db);
  
  uint32_t external_timestamp = 0xFFFFFFFF - 5;
  uint32_t internal_timestamp = external_timestamp;
  bool next_is_cng = false;
  for (; external_timestamp != 5; ++external_timestamp) {
    
    if (next_is_cng) {
      
      EXPECT_EQ(internal_timestamp,
                scaler.ToInternal(external_timestamp, kRtpPayloadTypeCng));
      next_is_cng = false;
    } else {
      
      EXPECT_EQ(internal_timestamp,
                scaler.ToInternal(external_timestamp, kRtpPayloadTypeG722));
      next_is_cng = true;
    }
    
    EXPECT_EQ(external_timestamp, scaler.ToExternal(internal_timestamp));
    internal_timestamp += 2;
  }

  EXPECT_CALL(db, Die());  
}




TEST(TimestampScaler, TestG722Packet) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderG722;  
  static const uint8_t kRtpPayloadType = 17;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  uint32_t external_timestamp = 0xFFFFFFFF - 5;
  uint32_t internal_timestamp = external_timestamp;
  Packet packet;
  packet.header.payloadType = kRtpPayloadType;
  for (; external_timestamp != 5; ++external_timestamp) {
    packet.header.timestamp = external_timestamp;
    
    scaler.ToInternal(&packet);
    EXPECT_EQ(internal_timestamp, packet.header.timestamp);
    internal_timestamp += 2;
  }

  EXPECT_CALL(db, Die());  
}




TEST(TimestampScaler, TestG722PacketList) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderG722;  
  static const uint8_t kRtpPayloadType = 17;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  uint32_t external_timestamp = 0xFFFFFFFF - 5;
  uint32_t internal_timestamp = external_timestamp;
  Packet packet1;
  packet1.header.payloadType = kRtpPayloadType;
  packet1.header.timestamp = external_timestamp;
  Packet packet2;
  packet2.header.payloadType = kRtpPayloadType;
  packet2.header.timestamp = external_timestamp + 10;
  PacketList packet_list;
  packet_list.push_back(&packet1);
  packet_list.push_back(&packet2);

  scaler.ToInternal(&packet_list);
  EXPECT_EQ(internal_timestamp, packet1.header.timestamp);
  EXPECT_EQ(internal_timestamp + 20, packet2.header.timestamp);

  EXPECT_CALL(db, Die());  
}

TEST(TimestampScaler, TestG722Reset) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderG722;  
  static const uint8_t kRtpPayloadType = 17;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  uint32_t external_timestamp = 0xFFFFFFFF - 5;
  uint32_t internal_timestamp = external_timestamp;
  for (; external_timestamp != 5; ++external_timestamp) {
    
    EXPECT_EQ(internal_timestamp,
              scaler.ToInternal(external_timestamp, kRtpPayloadType));
    
    EXPECT_EQ(external_timestamp, scaler.ToExternal(internal_timestamp));
    internal_timestamp += 2;
  }
  
  
  scaler.Reset();
  internal_timestamp = external_timestamp;
  for (; external_timestamp != 15; ++external_timestamp) {
    
    EXPECT_EQ(internal_timestamp,
              scaler.ToInternal(external_timestamp, kRtpPayloadType));
    
    EXPECT_EQ(external_timestamp, scaler.ToExternal(internal_timestamp));
    internal_timestamp += 2;
  }

  EXPECT_CALL(db, Die());  
}

TEST(TimestampScaler, TestOpusLargeStep) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderOpus;  
  static const uint8_t kRtpPayloadType = 17;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  static const uint32_t kStep = 960;
  uint32_t external_timestamp = 0;
  
  external_timestamp = external_timestamp - 5 * kStep;
  uint32_t internal_timestamp = external_timestamp;
  for (; external_timestamp != 5 * kStep; external_timestamp += kStep) {
    
    EXPECT_EQ(internal_timestamp,
              scaler.ToInternal(external_timestamp, kRtpPayloadType));
    
    EXPECT_EQ(external_timestamp, scaler.ToExternal(internal_timestamp));
    
    internal_timestamp += 2 * kStep / 3;
  }

  EXPECT_CALL(db, Die());  
}

TEST(TimestampScaler, TestIsacFbLargeStep) {
  MockDecoderDatabase db;
  DecoderDatabase::DecoderInfo info;
  info.codec_type = kDecoderISACfb;  
  static const uint8_t kRtpPayloadType = 17;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillRepeatedly(Return(&info));

  TimestampScaler scaler(db);
  
  static const uint32_t kStep = 960;
  uint32_t external_timestamp = 0;
  
  external_timestamp = external_timestamp - 5 * kStep;
  uint32_t internal_timestamp = external_timestamp;
  for (; external_timestamp != 5 * kStep; external_timestamp += kStep) {
    
    EXPECT_EQ(internal_timestamp,
              scaler.ToInternal(external_timestamp, kRtpPayloadType));
    
    EXPECT_EQ(external_timestamp, scaler.ToExternal(internal_timestamp));
    
    internal_timestamp += 2 * kStep / 3;
  }

  EXPECT_CALL(db, Die());  
}

TEST(TimestampScaler, Failures) {
  static const uint8_t kRtpPayloadType = 17;
  MockDecoderDatabase db;
  EXPECT_CALL(db, GetDecoderInfo(kRtpPayloadType))
      .WillOnce(ReturnNull());  

  TimestampScaler scaler(db);
  uint32_t timestamp = 4711;  
  EXPECT_EQ(timestamp, scaler.ToInternal(timestamp, kRtpPayloadType));

  Packet* packet = NULL;
  scaler.ToInternal(packet);  

  EXPECT_CALL(db, Die());  
}

}  
