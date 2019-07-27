











#include "webrtc/modules/audio_coding/neteq/packet_buffer.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq/mock/mock_decoder_database.h"
#include "webrtc/modules/audio_coding/neteq/packet.h"

using ::testing::Return;
using ::testing::_;

namespace webrtc {


class PacketGenerator {
 public:
  PacketGenerator(uint16_t seq_no, uint32_t ts, uint8_t pt, int frame_size);
  virtual ~PacketGenerator() {}
  void Reset(uint16_t seq_no, uint32_t ts, uint8_t pt, int frame_size);
  Packet* NextPacket(int payload_size_bytes);

  uint16_t seq_no_;
  uint32_t ts_;
  uint8_t pt_;
  int frame_size_;
};

PacketGenerator::PacketGenerator(uint16_t seq_no, uint32_t ts, uint8_t pt,
                                 int frame_size) {
  Reset(seq_no, ts, pt, frame_size);
}

void PacketGenerator::Reset(uint16_t seq_no, uint32_t ts, uint8_t pt,
                            int frame_size) {
  seq_no_ = seq_no;
  ts_ = ts;
  pt_ = pt;
  frame_size_ = frame_size;
}

Packet* PacketGenerator::NextPacket(int payload_size_bytes) {
  Packet* packet = new Packet;
  packet->header.sequenceNumber = seq_no_;
  packet->header.timestamp = ts_;
  packet->header.payloadType = pt_;
  packet->header.markerBit = false;
  packet->header.ssrc = 0x12345678;
  packet->header.numCSRCs = 0;
  packet->header.paddingLength = 0;
  packet->payload_length = payload_size_bytes;
  packet->primary = true;
  packet->payload = new uint8_t[payload_size_bytes];
  ++seq_no_;
  ts_ += frame_size_;
  return packet;
}

struct PacketsToInsert {
  uint16_t sequence_number;
  uint32_t timestamp;
  uint8_t payload_type;
  bool primary;
  
  
  
  int extract_order;
};



TEST(PacketBuffer, CreateAndDestroy) {
  PacketBuffer* buffer = new PacketBuffer(10);  
  EXPECT_TRUE(buffer->Empty());
  delete buffer;
}

TEST(PacketBuffer, InsertPacket) {
  PacketBuffer buffer(10);  
  PacketGenerator gen(17u, 4711u, 0, 10);

  const int payload_len = 100;
  Packet* packet = gen.NextPacket(payload_len);

  EXPECT_EQ(0, buffer.InsertPacket(packet));
  uint32_t next_ts;
  EXPECT_EQ(PacketBuffer::kOK, buffer.NextTimestamp(&next_ts));
  EXPECT_EQ(4711u, next_ts);
  EXPECT_FALSE(buffer.Empty());
  EXPECT_EQ(1, buffer.NumPacketsInBuffer());
  const RTPHeader* hdr = buffer.NextRtpHeader();
  EXPECT_EQ(&(packet->header), hdr);  

  
  
}


TEST(PacketBuffer, FlushBuffer) {
  PacketBuffer buffer(10);  
  PacketGenerator gen(0, 0, 0, 10);
  const int payload_len = 10;

  
  for (int i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacket(packet));
  }
  EXPECT_EQ(10, buffer.NumPacketsInBuffer());
  EXPECT_FALSE(buffer.Empty());

  buffer.Flush();
  
  EXPECT_EQ(0, buffer.NumPacketsInBuffer());
  EXPECT_TRUE(buffer.Empty());
}


TEST(PacketBuffer, OverfillBuffer) {
  PacketBuffer buffer(10);  
  PacketGenerator gen(0, 0, 0, 10);

  
  const int payload_len = 10;
  int i;
  for (i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacket(packet));
  }
  EXPECT_EQ(10, buffer.NumPacketsInBuffer());
  uint32_t next_ts;
  EXPECT_EQ(PacketBuffer::kOK, buffer.NextTimestamp(&next_ts));
  EXPECT_EQ(0u, next_ts);  

  
  Packet* packet = gen.NextPacket(payload_len);
  EXPECT_EQ(PacketBuffer::kFlushed, buffer.InsertPacket(packet));
  EXPECT_EQ(1, buffer.NumPacketsInBuffer());
  EXPECT_EQ(PacketBuffer::kOK, buffer.NextTimestamp(&next_ts));
  
  EXPECT_EQ(packet->header.timestamp, next_ts);

  
  buffer.Flush();
}


TEST(PacketBuffer, InsertPacketList) {
  PacketBuffer buffer(10);  
  PacketGenerator gen(0, 0, 0, 10);
  PacketList list;
  const int payload_len = 10;

  
  for (int i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    list.push_back(packet);
  }

  MockDecoderDatabase decoder_database;
  EXPECT_CALL(decoder_database, IsComfortNoise(0))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(decoder_database, IsDtmf(0))
      .WillRepeatedly(Return(false));
  uint8_t current_pt = 0xFF;
  uint8_t current_cng_pt = 0xFF;
  EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacketList(&list,
                                                       decoder_database,
                                                       &current_pt,
                                                       &current_cng_pt));
  EXPECT_TRUE(list.empty());  
  EXPECT_EQ(10, buffer.NumPacketsInBuffer());
  EXPECT_EQ(0, current_pt);  
  EXPECT_EQ(0xFF, current_cng_pt);  

  buffer.Flush();  

  EXPECT_CALL(decoder_database, Die());  
}




TEST(PacketBuffer, InsertPacketListChangePayloadType) {
  PacketBuffer buffer(10);  
  PacketGenerator gen(0, 0, 0, 10);
  PacketList list;
  const int payload_len = 10;

  
  for (int i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    list.push_back(packet);
  }
  
  Packet* packet = gen.NextPacket(payload_len);
  packet->header.payloadType = 1;
  list.push_back(packet);


  MockDecoderDatabase decoder_database;
  EXPECT_CALL(decoder_database, IsComfortNoise(_))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(decoder_database, IsDtmf(_))
      .WillRepeatedly(Return(false));
  uint8_t current_pt = 0xFF;
  uint8_t current_cng_pt = 0xFF;
  EXPECT_EQ(PacketBuffer::kFlushed, buffer.InsertPacketList(&list,
                                                            decoder_database,
                                                            &current_pt,
                                                            &current_cng_pt));
  EXPECT_TRUE(list.empty());  
  EXPECT_EQ(1, buffer.NumPacketsInBuffer());  
  EXPECT_EQ(1, current_pt);  
  EXPECT_EQ(0xFF, current_cng_pt);  

  buffer.Flush();  

  EXPECT_CALL(decoder_database, Die());  
}

TEST(PacketBuffer, ExtractOrderRedundancy) {
  PacketBuffer buffer(100);  
  const int kPackets = 18;
  const int kFrameSize = 10;
  const int kPayloadLength = 10;

  PacketsToInsert packet_facts[kPackets] = {
    {0xFFFD, 0xFFFFFFD7, 0, true, 0},
    {0xFFFE, 0xFFFFFFE1, 0, true, 1},
    {0xFFFE, 0xFFFFFFD7, 1, false, -1},
    {0xFFFF, 0xFFFFFFEB, 0, true, 2},
    {0xFFFF, 0xFFFFFFE1, 1, false, -1},
    {0x0000, 0xFFFFFFF5, 0, true, 3},
    {0x0000, 0xFFFFFFEB, 1, false, -1},
    {0x0001, 0xFFFFFFFF, 0, true, 4},
    {0x0001, 0xFFFFFFF5, 1, false, -1},
    {0x0002, 0x0000000A, 0, true, 5},
    {0x0002, 0xFFFFFFFF, 1, false, -1},
    {0x0003, 0x0000000A, 1, false, -1},
    {0x0004, 0x0000001E, 0, true, 7},
    {0x0004, 0x00000014, 1, false, 6},
    {0x0005, 0x0000001E, 0, true, -1},
    {0x0005, 0x00000014, 1, false, -1},
    {0x0006, 0x00000028, 0, true, 8},
    {0x0006, 0x0000001E, 1, false, -1},
  };

  const int kExpectPacketsInBuffer = 9;

  std::vector<Packet*> expect_order(kExpectPacketsInBuffer);

  PacketGenerator gen(0, 0, 0, kFrameSize);

  for (int i = 0; i < kPackets; ++i) {
    gen.Reset(packet_facts[i].sequence_number,
              packet_facts[i].timestamp,
              packet_facts[i].payload_type,
              kFrameSize);
    Packet* packet = gen.NextPacket(kPayloadLength);
    packet->primary = packet_facts[i].primary;
    EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacket(packet));
    if (packet_facts[i].extract_order >= 0) {
      expect_order[packet_facts[i].extract_order] = packet;
    }
  }

  EXPECT_EQ(kExpectPacketsInBuffer, buffer.NumPacketsInBuffer());

  int drop_count;
  for (int i = 0; i < kExpectPacketsInBuffer; ++i) {
    Packet* packet = buffer.GetNextPacket(&drop_count);
    EXPECT_EQ(0, drop_count);
    EXPECT_EQ(packet, expect_order[i]);  
    delete[] packet->payload;
    delete packet;
  }
  EXPECT_TRUE(buffer.Empty());
}

TEST(PacketBuffer, DiscardPackets) {
  PacketBuffer buffer(100);  
  const uint16_t start_seq_no = 17;
  const uint32_t start_ts = 4711;
  const uint32_t ts_increment = 10;
  PacketGenerator gen(start_seq_no, start_ts, 0, ts_increment);
  PacketList list;
  const int payload_len = 10;

  
  for (int i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    buffer.InsertPacket(packet);
  }
  EXPECT_EQ(10, buffer.NumPacketsInBuffer());

  
  
  uint32_t current_ts = start_ts;
  for (int i = 0; i < 10; ++i) {
    uint32_t ts;
    EXPECT_EQ(PacketBuffer::kOK, buffer.NextTimestamp(&ts));
    EXPECT_EQ(current_ts, ts);
    EXPECT_EQ(PacketBuffer::kOK, buffer.DiscardNextPacket());
    current_ts += ts_increment;
  }
  EXPECT_TRUE(buffer.Empty());
}

TEST(PacketBuffer, Reordering) {
  PacketBuffer buffer(100);  
  const uint16_t start_seq_no = 17;
  const uint32_t start_ts = 4711;
  const uint32_t ts_increment = 10;
  PacketGenerator gen(start_seq_no, start_ts, 0, ts_increment);
  const int payload_len = 10;

  
  
  
  PacketList list;
  for (int i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    if (i % 2) {
      list.push_front(packet);
    } else {
      list.push_back(packet);
    }
  }

  MockDecoderDatabase decoder_database;
  EXPECT_CALL(decoder_database, IsComfortNoise(0))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(decoder_database, IsDtmf(0))
      .WillRepeatedly(Return(false));
  uint8_t current_pt = 0xFF;
  uint8_t current_cng_pt = 0xFF;

  EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacketList(&list,
                                                       decoder_database,
                                                       &current_pt,
                                                       &current_cng_pt));
  EXPECT_EQ(10, buffer.NumPacketsInBuffer());

  
  uint32_t current_ts = start_ts;
  for (int i = 0; i < 10; ++i) {
    Packet* packet = buffer.GetNextPacket(NULL);
    ASSERT_FALSE(packet == NULL);
    EXPECT_EQ(current_ts, packet->header.timestamp);
    current_ts += ts_increment;
    delete [] packet->payload;
    delete packet;
  }
  EXPECT_TRUE(buffer.Empty());

  EXPECT_CALL(decoder_database, Die());  
}

TEST(PacketBuffer, Failures) {
  const uint16_t start_seq_no = 17;
  const uint32_t start_ts = 4711;
  const uint32_t ts_increment = 10;
  int payload_len = 100;
  PacketGenerator gen(start_seq_no, start_ts, 0, ts_increment);

  PacketBuffer* buffer = new PacketBuffer(100);  
  Packet* packet = NULL;
  EXPECT_EQ(PacketBuffer::kInvalidPacket, buffer->InsertPacket(packet));
  packet = gen.NextPacket(payload_len);
  delete [] packet->payload;
  packet->payload = NULL;
  EXPECT_EQ(PacketBuffer::kInvalidPacket, buffer->InsertPacket(packet));
  

  
  uint32_t temp_ts;
  EXPECT_EQ(PacketBuffer::kBufferEmpty, buffer->NextTimestamp(&temp_ts));
  EXPECT_EQ(PacketBuffer::kBufferEmpty,
            buffer->NextHigherTimestamp(0, &temp_ts));
  EXPECT_EQ(NULL, buffer->NextRtpHeader());
  EXPECT_EQ(NULL, buffer->GetNextPacket(NULL));
  EXPECT_EQ(PacketBuffer::kBufferEmpty, buffer->DiscardNextPacket());
  EXPECT_EQ(0, buffer->DiscardAllOldPackets(0));  

  
  packet = gen.NextPacket(payload_len);
  EXPECT_EQ(PacketBuffer::kOK, buffer->InsertPacket(packet));
  EXPECT_EQ(PacketBuffer::kInvalidPointer, buffer->NextTimestamp(NULL));
  EXPECT_EQ(PacketBuffer::kInvalidPointer,
            buffer->NextHigherTimestamp(0, NULL));
  delete buffer;

  
  
  
  buffer = new PacketBuffer(100);  
  PacketList list;
  list.push_back(gen.NextPacket(payload_len));  
  packet = gen.NextPacket(payload_len);
  delete [] packet->payload;
  packet->payload = NULL;  
  list.push_back(packet);
  list.push_back(gen.NextPacket(payload_len));  
  MockDecoderDatabase decoder_database;
  EXPECT_CALL(decoder_database, IsComfortNoise(0))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(decoder_database, IsDtmf(0))
      .WillRepeatedly(Return(false));
  uint8_t current_pt = 0xFF;
  uint8_t current_cng_pt = 0xFF;
  EXPECT_EQ(PacketBuffer::kInvalidPacket,
            buffer->InsertPacketList(&list,
                                     decoder_database,
                                     &current_pt,
                                     &current_cng_pt));
  EXPECT_TRUE(list.empty());  
  EXPECT_EQ(1, buffer->NumPacketsInBuffer());
  delete buffer;
  EXPECT_CALL(decoder_database, Die());  
}



TEST(PacketBuffer, ComparePackets) {
  PacketGenerator gen(0, 0, 0, 10);
  Packet* a = gen.NextPacket(10);  
  Packet* b = gen.NextPacket(10);  
  EXPECT_FALSE(*a == *b);
  EXPECT_TRUE(*a != *b);
  EXPECT_TRUE(*a < *b);
  EXPECT_FALSE(*a > *b);
  EXPECT_TRUE(*a <= *b);
  EXPECT_FALSE(*a >= *b);

  
  a->header.timestamp = 0xFFFFFFFF - 10;
  EXPECT_FALSE(*a == *b);
  EXPECT_TRUE(*a != *b);
  EXPECT_TRUE(*a < *b);
  EXPECT_FALSE(*a > *b);
  EXPECT_TRUE(*a <= *b);
  EXPECT_FALSE(*a >= *b);

  
  EXPECT_TRUE(*a == *a);
  EXPECT_FALSE(*a != *a);
  EXPECT_FALSE(*a < *a);
  EXPECT_FALSE(*a > *a);
  EXPECT_TRUE(*a <= *a);
  EXPECT_TRUE(*a >= *a);

  
  a->header.timestamp = b->header.timestamp;
  EXPECT_FALSE(*a == *b);
  EXPECT_TRUE(*a != *b);
  EXPECT_TRUE(*a < *b);
  EXPECT_FALSE(*a > *b);
  EXPECT_TRUE(*a <= *b);
  EXPECT_FALSE(*a >= *b);

  
  a->header.sequenceNumber = 0xFFFF;
  EXPECT_FALSE(*a == *b);
  EXPECT_TRUE(*a != *b);
  EXPECT_TRUE(*a < *b);
  EXPECT_FALSE(*a > *b);
  EXPECT_TRUE(*a <= *b);
  EXPECT_FALSE(*a >= *b);

  
  a->header.sequenceNumber = b->header.sequenceNumber;
  a->primary = false;
  b->primary = true;
  EXPECT_FALSE(*a == *b);
  EXPECT_TRUE(*a != *b);
  EXPECT_FALSE(*a < *b);
  EXPECT_TRUE(*a > *b);
  EXPECT_FALSE(*a <= *b);
  EXPECT_TRUE(*a >= *b);

  delete [] a->payload;
  delete a;
  delete [] b->payload;
  delete b;
}


TEST(PacketBuffer, DeleteAllPackets) {
  PacketGenerator gen(0, 0, 0, 10);
  PacketList list;
  const int payload_len = 10;

  
  for (int i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    list.push_back(packet);
  }
  EXPECT_TRUE(PacketBuffer::DeleteFirstPacket(&list));
  EXPECT_EQ(9u, list.size());
  PacketBuffer::DeleteAllPackets(&list);
  EXPECT_TRUE(list.empty());
  EXPECT_FALSE(PacketBuffer::DeleteFirstPacket(&list));
}

namespace {
void TestIsObsoleteTimestamp(uint32_t limit_timestamp) {
  
  
  static const uint32_t kZeroHorizon = 0;
  static const uint32_t k2Pow31Minus1 = 0x7FFFFFFF;
  
  EXPECT_FALSE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp, limit_timestamp, kZeroHorizon));
  
  EXPECT_TRUE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp - 1, limit_timestamp, kZeroHorizon));
  
  EXPECT_TRUE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp - k2Pow31Minus1, limit_timestamp, kZeroHorizon));
  
  EXPECT_FALSE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp + 1, limit_timestamp, kZeroHorizon));
  
  EXPECT_FALSE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp + (1 << 31), limit_timestamp, kZeroHorizon));

  
  static const uint32_t kHorizon = 10;
  
  EXPECT_FALSE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp, limit_timestamp, kHorizon));
  
  EXPECT_TRUE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp - 1, limit_timestamp, kHorizon));
  
  EXPECT_TRUE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp - 9, limit_timestamp, kHorizon));
  
  EXPECT_FALSE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp - 10, limit_timestamp, kHorizon));
  
  EXPECT_FALSE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp - k2Pow31Minus1, limit_timestamp, kHorizon));
  
  EXPECT_FALSE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp + 1, limit_timestamp, kHorizon));
  
  EXPECT_FALSE(PacketBuffer::IsObsoleteTimestamp(
      limit_timestamp + (1 << 31), limit_timestamp, kHorizon));
}
}  


TEST(PacketBuffer, IsObsoleteTimestamp) {
  TestIsObsoleteTimestamp(0);
  TestIsObsoleteTimestamp(1);
  TestIsObsoleteTimestamp(0xFFFFFFFF);  
  TestIsObsoleteTimestamp(0x80000000);  
  TestIsObsoleteTimestamp(0x80000001);  
  TestIsObsoleteTimestamp(0x7FFFFFFF);  
}
}  
