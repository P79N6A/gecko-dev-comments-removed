











#include "webrtc/modules/audio_coding/neteq4/packet_buffer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/mock/mock_decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/packet.h"

using ::testing::Return;
using ::testing::_;

namespace webrtc {


class PacketGenerator {
 public:
  PacketGenerator(uint16_t seq_no, uint32_t ts, uint8_t pt, int frame_size);
  virtual ~PacketGenerator() {}
  Packet* NextPacket(int payload_size_bytes);
  void SkipPacket();

  uint16_t seq_no_;
  uint32_t ts_;
  uint8_t pt_;
  int frame_size_;
};

PacketGenerator::PacketGenerator(uint16_t seq_no, uint32_t ts, uint8_t pt,
                                 int frame_size)
    : seq_no_(seq_no),
      ts_(ts),
      pt_(pt),
      frame_size_(frame_size) {
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

void PacketGenerator::SkipPacket() {
  ++seq_no_;
  ts_ += frame_size_;
}




TEST(PacketBuffer, CreateAndDestroy) {
  PacketBuffer* buffer = new PacketBuffer(10, 1000);  
  EXPECT_TRUE(buffer->Empty());
  delete buffer;
}

TEST(PacketBuffer, InsertPacket) {
  PacketBuffer buffer(10, 1000);  
  PacketGenerator gen(17u, 4711u, 0, 10);

  const int payload_len = 100;
  Packet* packet = gen.NextPacket(payload_len);

  EXPECT_EQ(0, buffer.InsertPacket(packet));
  uint32_t next_ts;
  EXPECT_EQ(PacketBuffer::kOK, buffer.NextTimestamp(&next_ts));
  EXPECT_EQ(4711u, next_ts);
  EXPECT_FALSE(buffer.Empty());
  EXPECT_EQ(1, buffer.NumPacketsInBuffer());
  EXPECT_EQ(payload_len, buffer.current_memory_bytes());
  const RTPHeader* hdr = buffer.NextRtpHeader();
  EXPECT_EQ(&(packet->header), hdr);  

  
  
}


TEST(PacketBuffer, FlushBuffer) {
  PacketBuffer buffer(10, 1000);  
  PacketGenerator gen(0, 0, 0, 10);
  const int payload_len = 10;

  
  for (int i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacket(packet));
  }
  EXPECT_EQ(10, buffer.NumPacketsInBuffer());
  EXPECT_FALSE(buffer.Empty());
  EXPECT_EQ(10 * payload_len, buffer.current_memory_bytes());

  buffer.Flush();
  
  EXPECT_EQ(0, buffer.NumPacketsInBuffer());
  EXPECT_TRUE(buffer.Empty());
  EXPECT_EQ(0, buffer.current_memory_bytes());
}


TEST(PacketBuffer, OverfillBuffer) {
  PacketBuffer buffer(10, 1000);  
  PacketGenerator gen(0, 0, 0, 10);

  
  const int payload_len = 10;
  int i;
  for (i = 0; i < 10; ++i) {
    Packet* packet = gen.NextPacket(payload_len);
    EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacket(packet));
  }
  EXPECT_EQ(10, buffer.NumPacketsInBuffer());
  EXPECT_EQ(10 * payload_len, buffer.current_memory_bytes());
  uint32_t next_ts;
  EXPECT_EQ(PacketBuffer::kOK, buffer.NextTimestamp(&next_ts));
  EXPECT_EQ(0u, next_ts);  

  
  Packet* packet = gen.NextPacket(payload_len);
  EXPECT_EQ(PacketBuffer::kFlushed, buffer.InsertPacket(packet));
  EXPECT_EQ(1, buffer.NumPacketsInBuffer());
  EXPECT_EQ(payload_len, buffer.current_memory_bytes());
  EXPECT_EQ(PacketBuffer::kOK, buffer.NextTimestamp(&next_ts));
  
  EXPECT_EQ(packet->header.timestamp, next_ts);

  
  const int large_payload_len = 500;
  packet = gen.NextPacket(large_payload_len);
  EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacket(packet));
  EXPECT_EQ(2, buffer.NumPacketsInBuffer());
  EXPECT_EQ(payload_len + large_payload_len, buffer.current_memory_bytes());

  packet = gen.NextPacket(large_payload_len);
  EXPECT_EQ(PacketBuffer::kFlushed, buffer.InsertPacket(packet));
  EXPECT_EQ(1, buffer.NumPacketsInBuffer());
  EXPECT_EQ(large_payload_len, buffer.current_memory_bytes());

  
  buffer.Flush();
}


TEST(PacketBuffer, InsertPacketList) {
  PacketBuffer buffer(10, 1000);  
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
  EXPECT_EQ(10 * payload_len, buffer.current_memory_bytes());
  EXPECT_EQ(0, current_pt);  
  EXPECT_EQ(0xFF, current_cng_pt);  

  buffer.Flush();  

  EXPECT_CALL(decoder_database, Die());  
}




TEST(PacketBuffer, InsertPacketListChangePayloadType) {
  PacketBuffer buffer(10, 1000);  
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
  EXPECT_EQ(1 * payload_len, buffer.current_memory_bytes());
  EXPECT_EQ(1, current_pt);  
  EXPECT_EQ(0xFF, current_cng_pt);  

  buffer.Flush();  

  EXPECT_CALL(decoder_database, Die());  
}














TEST(PacketBuffer, ExtractOrderRedundancy) {
  PacketBuffer buffer(100, 1000);  
  const uint32_t ts_increment = 10;  
  const uint16_t start_seq_no = 0xFFFF - 2;  
  const uint32_t start_ts = 0xFFFFFFFF -
      4 * ts_increment;  
  const uint8_t primary_pt = 0;
  const uint8_t secondary_pt = 1;
  PacketGenerator gen(start_seq_no, start_ts, primary_pt, ts_increment);
  
  PacketGenerator red_gen(start_seq_no + 1, start_ts, secondary_pt,
                          ts_increment);

  
  for (int i = 0; i < 10; ++i) {
    const int payload_len = 10;
    if (i == 6) {
      
      gen.SkipPacket();
      red_gen.SkipPacket();
      continue;
    }
    
    Packet* packet = gen.NextPacket(payload_len);
    EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacket(packet));
    if (i >= 1) {
      
      packet = red_gen.NextPacket(payload_len);
      packet->primary = false;
      EXPECT_EQ(PacketBuffer::kOK, buffer.InsertPacket(packet));
    }
  }
  EXPECT_EQ(17, buffer.NumPacketsInBuffer());  

  uint16_t current_seq_no = start_seq_no;
  uint32_t current_ts = start_ts;

  for (int i = 0; i < 10; ++i) {
    
    int drop_count = 0;
    Packet* packet = buffer.GetNextPacket(&drop_count);
    ASSERT_FALSE(packet == NULL);
    if (i == 6) {
      
      
      EXPECT_EQ(current_seq_no + 1, packet->header.sequenceNumber);
      EXPECT_EQ(current_ts, packet->header.timestamp);
      EXPECT_FALSE(packet->primary);
      EXPECT_EQ(1, packet->header.payloadType);
      EXPECT_EQ(0, drop_count);
    } else {
      EXPECT_EQ(current_seq_no, packet->header.sequenceNumber);
      EXPECT_EQ(current_ts, packet->header.timestamp);
      EXPECT_TRUE(packet->primary);
      EXPECT_EQ(0, packet->header.payloadType);
      if (i == 5 || i == 9) {
        
        EXPECT_EQ(0, drop_count);
      } else {
        EXPECT_EQ(1, drop_count);
      }
    }
    ++current_seq_no;
    current_ts += ts_increment;
    delete [] packet->payload;
    delete packet;
  }
}

TEST(PacketBuffer, DiscardPackets) {
  PacketBuffer buffer(100, 1000);  
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
  EXPECT_EQ(10 * payload_len, buffer.current_memory_bytes());

  
  
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
  PacketBuffer buffer(100, 1000);  
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
  EXPECT_EQ(10 * payload_len, buffer.current_memory_bytes());

  
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

  PacketBuffer* buffer = new PacketBuffer(0, 1000);  
  Packet* packet = gen.NextPacket(payload_len);
  EXPECT_EQ(PacketBuffer::kOversizePacket, buffer->InsertPacket(packet));
  delete buffer;

  buffer = new PacketBuffer(100, 10);  
  packet = gen.NextPacket(payload_len);
  EXPECT_EQ(PacketBuffer::kOversizePacket, buffer->InsertPacket(packet));
  delete buffer;

  buffer = new PacketBuffer(100, 10000);  
  packet = NULL;
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
  EXPECT_EQ(0, buffer->DiscardOldPackets(0));  

  
  packet = gen.NextPacket(payload_len);
  EXPECT_EQ(PacketBuffer::kOK, buffer->InsertPacket(packet));
  EXPECT_EQ(PacketBuffer::kInvalidPointer, buffer->NextTimestamp(NULL));
  EXPECT_EQ(PacketBuffer::kInvalidPointer,
            buffer->NextHigherTimestamp(0, NULL));
  delete buffer;

  
  
  
  buffer = new PacketBuffer(100, 1000);  
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

}  
