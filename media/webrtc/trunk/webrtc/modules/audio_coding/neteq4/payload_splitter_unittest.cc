











#include "webrtc/modules/audio_coding/neteq4/payload_splitter.h"

#include <assert.h>

#include <utility>  

#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/mock/mock_decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/packet.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

using ::testing::Return;
using ::testing::ReturnNull;

namespace webrtc {

static const int kRedPayloadType = 100;
static const int kPayloadLength = 10;
static const int kRedHeaderLength = 4;  
static const uint16_t kSequenceNumber = 0;
static const uint32_t kBaseTimestamp = 0x12345678;



















Packet* CreateRedPayload(int num_payloads,
                         uint8_t* payload_types,
                         int timestamp_offset) {
  Packet* packet = new Packet;
  packet->header.payloadType = kRedPayloadType;
  packet->header.timestamp = kBaseTimestamp;
  packet->header.sequenceNumber = kSequenceNumber;
  packet->payload_length = (kPayloadLength + 1) +
      (num_payloads - 1) * (kPayloadLength + kRedHeaderLength);
  uint8_t* payload = new uint8_t[packet->payload_length];
  uint8_t* payload_ptr = payload;
  for (int i = 0; i < num_payloads; ++i) {
    
    if (i == num_payloads - 1) {
      
      *payload_ptr = payload_types[i] & 0x7F;  
      ++payload_ptr;
      break;
    }
    *payload_ptr = payload_types[i] & 0x7F;
    
    *payload_ptr |= 0x80;
    ++payload_ptr;
    int this_offset = (num_payloads - i - 1) * timestamp_offset;
    *payload_ptr = this_offset >> 6;
    ++payload_ptr;
    assert(kPayloadLength <= 1023);  
    *payload_ptr = ((this_offset & 0x3F) << 2) | (kPayloadLength >> 8);
    ++payload_ptr;
    *payload_ptr = kPayloadLength & 0xFF;
    ++payload_ptr;
  }
  for (int i = 0; i < num_payloads; ++i) {
    
    memset(payload_ptr, i, kPayloadLength);
    payload_ptr += kPayloadLength;
  }
  packet->payload = payload;
  return packet;
}


Packet* CreatePacket(uint8_t payload_type, int payload_length,
                     uint8_t payload_value) {
  Packet* packet = new Packet;
  packet->header.payloadType = payload_type;
  packet->header.timestamp = kBaseTimestamp;
  packet->header.sequenceNumber = kSequenceNumber;
  packet->payload_length = payload_length;
  uint8_t* payload = new uint8_t[packet->payload_length];
  memset(payload, payload_value, payload_length);
  packet->payload = payload;
  return packet;
}


void VerifyPacket(const Packet* packet,
                  int payload_length,
                  uint8_t payload_type,
                  uint16_t sequence_number,
                  uint32_t timestamp,
                  uint8_t payload_value,
                  bool primary = true) {
  EXPECT_EQ(payload_length, packet->payload_length);
  EXPECT_EQ(payload_type, packet->header.payloadType);
  EXPECT_EQ(sequence_number, packet->header.sequenceNumber);
  EXPECT_EQ(timestamp, packet->header.timestamp);
  EXPECT_EQ(primary, packet->primary);
  ASSERT_FALSE(packet->payload == NULL);
  for (int i = 0; i < packet->payload_length; ++i) {
    EXPECT_EQ(payload_value, packet->payload[i]);
  }
}



TEST(PayloadSplitter, CreateAndDestroy) {
  PayloadSplitter* splitter = new PayloadSplitter;
  delete splitter;
}


TEST(RedPayloadSplitter, OnePacketTwoPayloads) {
  uint8_t payload_types[] = {0, 0};
  const int kTimestampOffset = 160;
  Packet* packet = CreateRedPayload(2, payload_types, kTimestampOffset);
  PacketList packet_list;
  packet_list.push_back(packet);
  PayloadSplitter splitter;
  EXPECT_EQ(PayloadSplitter::kOK, splitter.SplitRed(&packet_list));
  ASSERT_EQ(2u, packet_list.size());
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[1], kSequenceNumber,
               kBaseTimestamp, 1, true);
  delete [] packet->payload;
  delete packet;
  packet_list.pop_front();
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[0], kSequenceNumber,
               kBaseTimestamp - kTimestampOffset, 0, false);
  delete [] packet->payload;
  delete packet;
}



TEST(RedPayloadSplitter, TwoPacketsOnePayload) {
  uint8_t payload_types[] = {0};
  const int kTimestampOffset = 160;
  
  Packet* packet = CreateRedPayload(1, payload_types, kTimestampOffset);
  PacketList packet_list;
  packet_list.push_back(packet);
  
  packet = CreateRedPayload(1, payload_types, kTimestampOffset);
  
  packet->header.timestamp += kTimestampOffset;
  packet->header.sequenceNumber++;
  packet_list.push_back(packet);
  PayloadSplitter splitter;
  EXPECT_EQ(PayloadSplitter::kOK, splitter.SplitRed(&packet_list));
  ASSERT_EQ(2u, packet_list.size());
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[0], kSequenceNumber,
               kBaseTimestamp, 0, true);
  delete [] packet->payload;
  delete packet;
  packet_list.pop_front();
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[0], kSequenceNumber + 1,
               kBaseTimestamp + kTimestampOffset, 0, true);
  delete [] packet->payload;
  delete packet;
}










TEST(RedPayloadSplitter, TwoPacketsThreePayloads) {
  uint8_t payload_types[] = {2, 1, 0};  
  const int kTimestampOffset = 160;
  
  Packet* packet = CreateRedPayload(3, payload_types, kTimestampOffset);
  PacketList packet_list;
  packet_list.push_back(packet);
  
  packet = CreateRedPayload(3, payload_types, kTimestampOffset);
  
  packet->header.timestamp += kTimestampOffset;
  packet->header.sequenceNumber++;
  packet_list.push_back(packet);
  PayloadSplitter splitter;
  EXPECT_EQ(PayloadSplitter::kOK, splitter.SplitRed(&packet_list));
  ASSERT_EQ(6u, packet_list.size());
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[2], kSequenceNumber,
               kBaseTimestamp, 2, true);
  delete [] packet->payload;
  delete packet;
  packet_list.pop_front();
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[1], kSequenceNumber,
               kBaseTimestamp - kTimestampOffset, 1, false);
  delete [] packet->payload;
  delete packet;
  packet_list.pop_front();
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[0], kSequenceNumber,
               kBaseTimestamp - 2 * kTimestampOffset, 0, false);
  delete [] packet->payload;
  delete packet;
  packet_list.pop_front();
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[2], kSequenceNumber + 1,
               kBaseTimestamp + kTimestampOffset, 2, true);
  delete [] packet->payload;
  delete packet;
  packet_list.pop_front();
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[1], kSequenceNumber + 1,
               kBaseTimestamp, 1, false);
  delete [] packet->payload;
  delete packet;
  packet_list.pop_front();
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[0], kSequenceNumber + 1,
               kBaseTimestamp - kTimestampOffset, 0, false);
  delete [] packet->payload;
  delete packet;
}









TEST(RedPayloadSplitter, CheckRedPayloads) {
  PacketList packet_list;
  for (int i = 0; i <= 3; ++i) {
    
    Packet* packet = CreatePacket(i, 10, 0);
    packet_list.push_back(packet);
  }

  
  
  
  DecoderDatabase decoder_database;
  decoder_database.RegisterPayload(0, kDecoderCNGnb);
  decoder_database.RegisterPayload(1, kDecoderPCMu);
  decoder_database.RegisterPayload(2, kDecoderAVT);
  decoder_database.RegisterPayload(3, kDecoderILBC);

  PayloadSplitter splitter;
  splitter.CheckRedPayloads(&packet_list, decoder_database);

  ASSERT_EQ(3u, packet_list.size());  
  
  
  for (int i = 0; i <= 2; ++i) {
    Packet* packet = packet_list.front();
    VerifyPacket(packet, 10, i, kSequenceNumber, kBaseTimestamp, 0, true);
    delete [] packet->payload;
    delete packet;
    packet_list.pop_front();
  }
  EXPECT_TRUE(packet_list.empty());
}



TEST(RedPayloadSplitter, WrongPayloadLength) {
  uint8_t payload_types[] = {0, 0, 0};
  const int kTimestampOffset = 160;
  Packet* packet = CreateRedPayload(3, payload_types, kTimestampOffset);
  
  
  
  packet->payload_length -= kPayloadLength + 1;
  PacketList packet_list;
  packet_list.push_back(packet);
  PayloadSplitter splitter;
  EXPECT_EQ(PayloadSplitter::kRedLengthMismatch,
            splitter.SplitRed(&packet_list));
  ASSERT_EQ(1u, packet_list.size());
  
  packet = packet_list.front();
  VerifyPacket(packet, kPayloadLength, payload_types[0], kSequenceNumber,
               kBaseTimestamp - 2 * kTimestampOffset, 0, false);
  delete [] packet->payload;
  delete packet;
  packet_list.pop_front();
}



TEST(AudioPayloadSplitter, NonSplittable) {
  
  
  PacketList packet_list;
  for (int i = 0; i < 6; ++i) {
    
    packet_list.push_back(CreatePacket(i, kPayloadLength, 10 * i));
  }

  MockDecoderDatabase decoder_database;
  
  
  
  scoped_ptr<DecoderDatabase::DecoderInfo> info0(
      new DecoderDatabase::DecoderInfo(kDecoderISAC, 16000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(0))
      .WillRepeatedly(Return(info0.get()));
  scoped_ptr<DecoderDatabase::DecoderInfo> info1(
      new DecoderDatabase::DecoderInfo(kDecoderISACswb, 32000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(1))
      .WillRepeatedly(Return(info1.get()));
  scoped_ptr<DecoderDatabase::DecoderInfo> info2(
      new DecoderDatabase::DecoderInfo(kDecoderRED, 8000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(2))
      .WillRepeatedly(Return(info2.get()));
  scoped_ptr<DecoderDatabase::DecoderInfo> info3(
      new DecoderDatabase::DecoderInfo(kDecoderAVT, 8000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(3))
      .WillRepeatedly(Return(info3.get()));
  scoped_ptr<DecoderDatabase::DecoderInfo> info4(
      new DecoderDatabase::DecoderInfo(kDecoderCNGnb, 8000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(4))
      .WillRepeatedly(Return(info4.get()));
  scoped_ptr<DecoderDatabase::DecoderInfo> info5(
      new DecoderDatabase::DecoderInfo(kDecoderArbitrary, 8000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(5))
      .WillRepeatedly(Return(info5.get()));

  PayloadSplitter splitter;
  EXPECT_EQ(0, splitter.SplitAudio(&packet_list, decoder_database));
  EXPECT_EQ(6u, packet_list.size());

  
  uint8_t payload_type = 0;
  PacketList::iterator it = packet_list.begin();
  while (it != packet_list.end()) {
    VerifyPacket((*it), kPayloadLength, payload_type, kSequenceNumber,
                 kBaseTimestamp, 10 * payload_type);
    ++payload_type;
    delete [] (*it)->payload;
    delete (*it);
    it = packet_list.erase(it);
  }

  
  EXPECT_CALL(decoder_database, Die());
}


TEST(AudioPayloadSplitter, UnknownPayloadType) {
  PacketList packet_list;
  static const uint8_t kPayloadType = 17;  
  int kPayloadLengthBytes = 4711;  
  packet_list.push_back(CreatePacket(kPayloadType, kPayloadLengthBytes, 0));

  MockDecoderDatabase decoder_database;
  
  
  EXPECT_CALL(decoder_database, GetDecoderInfo(kPayloadType))
      .WillRepeatedly(ReturnNull());

  PayloadSplitter splitter;
  EXPECT_EQ(PayloadSplitter::kUnknownPayloadType,
            splitter.SplitAudio(&packet_list, decoder_database));
  EXPECT_EQ(1u, packet_list.size());


  
  PacketList::iterator it = packet_list.begin();
  while (it != packet_list.end()) {
    delete [] (*it)->payload;
    delete (*it);
    it = packet_list.erase(it);
  }

  
  EXPECT_CALL(decoder_database, Die());
}

class SplitBySamplesTest : public ::testing::TestWithParam<NetEqDecoder> {
 protected:
  virtual void SetUp() {
    decoder_type_ = GetParam();
    switch (decoder_type_) {
      case kDecoderPCMu:
      case kDecoderPCMa:
        bytes_per_ms_ = 8;
        samples_per_ms_ = 8;
        break;
      case kDecoderPCMu_2ch:
      case kDecoderPCMa_2ch:
        bytes_per_ms_ = 2 * 8;
        samples_per_ms_ = 8;
        break;
      case kDecoderG722:
        bytes_per_ms_ = 8;
        samples_per_ms_ = 16;
        break;
      case kDecoderPCM16B:
        bytes_per_ms_ = 16;
        samples_per_ms_ = 8;
        break;
      case kDecoderPCM16Bwb:
        bytes_per_ms_ = 32;
        samples_per_ms_ = 16;
        break;
      case kDecoderPCM16Bswb32kHz:
        bytes_per_ms_ = 64;
        samples_per_ms_ = 32;
        break;
      case kDecoderPCM16Bswb48kHz:
        bytes_per_ms_ = 96;
        samples_per_ms_ = 48;
        break;
      case kDecoderPCM16B_2ch:
        bytes_per_ms_ = 2 * 16;
        samples_per_ms_ = 8;
        break;
      case kDecoderPCM16Bwb_2ch:
        bytes_per_ms_ = 2 * 32;
        samples_per_ms_ = 16;
        break;
      case kDecoderPCM16Bswb32kHz_2ch:
        bytes_per_ms_ = 2 * 64;
        samples_per_ms_ = 32;
        break;
      case kDecoderPCM16Bswb48kHz_2ch:
        bytes_per_ms_ = 2 * 96;
        samples_per_ms_ = 48;
        break;
      case kDecoderPCM16B_5ch:
        bytes_per_ms_ = 5 * 16;
        samples_per_ms_ = 8;
        break;
      default:
        assert(false);
        break;
    }
  }
  int bytes_per_ms_;
  int samples_per_ms_;
  NetEqDecoder decoder_type_;
};


TEST_P(SplitBySamplesTest, PayloadSizes) {
  PacketList packet_list;
  static const uint8_t kPayloadType = 17;  
  for (int payload_size_ms = 10; payload_size_ms <= 60; payload_size_ms += 10) {
    
    
    int payload_size_bytes = payload_size_ms * bytes_per_ms_;
    packet_list.push_back(CreatePacket(kPayloadType, payload_size_bytes,
                                       payload_size_ms));
  }

  MockDecoderDatabase decoder_database;
  
  
  
  
  scoped_ptr<DecoderDatabase::DecoderInfo> info(
      new DecoderDatabase::DecoderInfo(decoder_type_, 8000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(kPayloadType))
      .WillRepeatedly(Return(info.get()));

  PayloadSplitter splitter;
  EXPECT_EQ(0, splitter.SplitAudio(&packet_list, decoder_database));
  
  
  
  
  
  
  
  int expected_size_ms[] = {10, 20, 30, 20, 20, 25, 25, 30, 30};
  int expected_payload_value[] = {10, 20, 30, 40, 40, 50, 50, 60, 60};
  int expected_timestamp_offset_ms[] = {0, 0, 0, 0, 20, 0, 25, 0, 30};
  size_t expected_num_packets =
      sizeof(expected_size_ms) / sizeof(expected_size_ms[0]);
  EXPECT_EQ(expected_num_packets, packet_list.size());

  PacketList::iterator it = packet_list.begin();
  int i = 0;
  while (it != packet_list.end()) {
    int length_bytes = expected_size_ms[i] * bytes_per_ms_;
    uint32_t expected_timestamp = kBaseTimestamp +
        expected_timestamp_offset_ms[i] * samples_per_ms_;
    VerifyPacket((*it), length_bytes, kPayloadType, kSequenceNumber,
                 expected_timestamp, expected_payload_value[i]);
    delete [] (*it)->payload;
    delete (*it);
    it = packet_list.erase(it);
    ++i;
  }

  
  EXPECT_CALL(decoder_database, Die());
}

INSTANTIATE_TEST_CASE_P(
    PayloadSplitter, SplitBySamplesTest,
    ::testing::Values(kDecoderPCMu, kDecoderPCMa, kDecoderPCMu_2ch,
                      kDecoderPCMa_2ch, kDecoderG722, kDecoderPCM16B,
                      kDecoderPCM16Bwb, kDecoderPCM16Bswb32kHz,
                      kDecoderPCM16Bswb48kHz, kDecoderPCM16B_2ch,
                      kDecoderPCM16Bwb_2ch, kDecoderPCM16Bswb32kHz_2ch,
                      kDecoderPCM16Bswb48kHz_2ch, kDecoderPCM16B_5ch));


class SplitIlbcTest : public ::testing::TestWithParam<std::pair<int, int> > {
 protected:
  virtual void SetUp() {
    const std::pair<int, int> parameters = GetParam();
    num_frames_ = parameters.first;
    frame_length_ms_ = parameters.second;
    frame_length_bytes_ = (frame_length_ms_ == 20) ? 38 : 50;
  }
  size_t num_frames_;
  int frame_length_ms_;
  int frame_length_bytes_;
};


TEST_P(SplitIlbcTest, NumFrames) {
  PacketList packet_list;
  static const uint8_t kPayloadType = 17;  
  const int frame_length_samples = frame_length_ms_ * 8;
  int payload_length_bytes = frame_length_bytes_ * num_frames_;
  Packet* packet = CreatePacket(kPayloadType, payload_length_bytes, 0);
  
  for (int i = 0; i < packet->payload_length; ++i) {
    packet->payload[i] = static_cast<uint8_t>(i);
  }
  packet_list.push_back(packet);

  MockDecoderDatabase decoder_database;
  
  
  
  scoped_ptr<DecoderDatabase::DecoderInfo> info(
      new DecoderDatabase::DecoderInfo(kDecoderILBC, 8000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(kPayloadType))
      .WillRepeatedly(Return(info.get()));

  PayloadSplitter splitter;
  EXPECT_EQ(0, splitter.SplitAudio(&packet_list, decoder_database));
  EXPECT_EQ(num_frames_, packet_list.size());

  PacketList::iterator it = packet_list.begin();
  int frame_num = 0;
  uint8_t payload_value = 0;
  while (it != packet_list.end()) {
    Packet* packet = (*it);
    EXPECT_EQ(kBaseTimestamp + frame_length_samples * frame_num,
              packet->header.timestamp);
    EXPECT_EQ(frame_length_bytes_, packet->payload_length);
    EXPECT_EQ(kPayloadType, packet->header.payloadType);
    EXPECT_EQ(kSequenceNumber, packet->header.sequenceNumber);
    EXPECT_EQ(true, packet->primary);
    ASSERT_FALSE(packet->payload == NULL);
    for (int i = 0; i < packet->payload_length; ++i) {
      EXPECT_EQ(payload_value, packet->payload[i]);
      ++payload_value;
    }
    delete [] (*it)->payload;
    delete (*it);
    it = packet_list.erase(it);
    ++frame_num;
  }

  
  EXPECT_CALL(decoder_database, Die());
}





INSTANTIATE_TEST_CASE_P(
    PayloadSplitter, SplitIlbcTest,
    ::testing::Values(std::pair<int, int>(1, 20),  
                      std::pair<int, int>(2, 20),  
                      std::pair<int, int>(3, 20),  
                      std::pair<int, int>(4, 20),
                      std::pair<int, int>(5, 20),
                      std::pair<int, int>(24, 20),
                      std::pair<int, int>(1, 30),
                      std::pair<int, int>(2, 30),
                      std::pair<int, int>(3, 30),
                      std::pair<int, int>(4, 30),
                      std::pair<int, int>(5, 30),
                      std::pair<int, int>(18, 30)));


TEST(IlbcPayloadSplitter, TooLargePayload) {
  PacketList packet_list;
  static const uint8_t kPayloadType = 17;  
  int kPayloadLengthBytes = 950;
  Packet* packet = CreatePacket(kPayloadType, kPayloadLengthBytes, 0);
  packet_list.push_back(packet);

  MockDecoderDatabase decoder_database;
  scoped_ptr<DecoderDatabase::DecoderInfo> info(
      new DecoderDatabase::DecoderInfo(kDecoderILBC, 8000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(kPayloadType))
      .WillRepeatedly(Return(info.get()));

  PayloadSplitter splitter;
  EXPECT_EQ(PayloadSplitter::kTooLargePayload,
            splitter.SplitAudio(&packet_list, decoder_database));
  EXPECT_EQ(1u, packet_list.size());

  
  PacketList::iterator it = packet_list.begin();
  while (it != packet_list.end()) {
    delete [] (*it)->payload;
    delete (*it);
    it = packet_list.erase(it);
  }

  
  EXPECT_CALL(decoder_database, Die());
}


TEST(IlbcPayloadSplitter, UnevenPayload) {
  PacketList packet_list;
  static const uint8_t kPayloadType = 17;  
  int kPayloadLengthBytes = 39;  
  Packet* packet = CreatePacket(kPayloadType, kPayloadLengthBytes, 0);
  packet_list.push_back(packet);

  MockDecoderDatabase decoder_database;
  scoped_ptr<DecoderDatabase::DecoderInfo> info(
      new DecoderDatabase::DecoderInfo(kDecoderILBC, 8000, NULL, false));
  EXPECT_CALL(decoder_database, GetDecoderInfo(kPayloadType))
      .WillRepeatedly(Return(info.get()));

  PayloadSplitter splitter;
  EXPECT_EQ(PayloadSplitter::kFrameSplitError,
            splitter.SplitAudio(&packet_list, decoder_database));
  EXPECT_EQ(1u, packet_list.size());

  
  PacketList::iterator it = packet_list.begin();
  while (it != packet_list.end()) {
    delete [] (*it)->payload;
    delete (*it);
    it = packet_list.erase(it);
  }

  
  EXPECT_CALL(decoder_database, Die());
}

}  
