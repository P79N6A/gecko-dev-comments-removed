









#include <string.h>

#include <list>

#include "gtest/gtest.h"
#include "modules/video_coding/main/source/jitter_buffer.h"
#include "modules/video_coding/main/source/media_opt_util.h"
#include "modules/video_coding/main/source/mock/fake_tick_time.h"
#include "modules/video_coding/main/source/packet.h"

namespace webrtc {

class StreamGenerator {
 public:
  StreamGenerator(uint16_t start_seq_num, uint32_t start_timestamp,
                  int64_t current_time)
      : packets_(),
        sequence_number_(start_seq_num),
        timestamp_(start_timestamp),
        start_time_(current_time) {}

  void Init(uint16_t start_seq_num, uint32_t start_timestamp,
            int64_t current_time) {
    packets_.clear();
    sequence_number_ = start_seq_num;
    timestamp_ = start_timestamp;
    start_time_ = current_time;
  }

  void GenerateFrame(FrameType type, int num_media_packets,
                     int num_empty_packets, int64_t current_time) {
    timestamp_ += 90 * (current_time - start_time_);
    
    
    sequence_number_ += packets_.size();
    packets_.clear();
    for (int i = 0; i < num_media_packets; ++i) {
      packets_.push_back(GeneratePacket(sequence_number_,
                                        timestamp_,
                                        (i == 0),
                                        (i == num_media_packets - 1),
                                        type));
      ++sequence_number_;
    }
    for (int i = 0; i < num_empty_packets; ++i) {
      packets_.push_back(GeneratePacket(sequence_number_,
                                        timestamp_,
                                        false,
                                        false,
                                        kFrameEmpty));
      ++sequence_number_;
    }
  }

  static VCMPacket GeneratePacket(uint16_t sequence_number,
                                  uint32_t timestamp,
                                  bool first_packet,
                                  bool marker_bit,
                                  FrameType type) {
    VCMPacket packet;
    packet.seqNum = sequence_number;
    packet.timestamp = timestamp;
    packet.frameType = type;
    packet.isFirstPacket = first_packet;
    packet.markerBit = marker_bit;
    if (packet.isFirstPacket)
      packet.completeNALU = kNaluStart;
    else if (packet.markerBit)
      packet.completeNALU = kNaluEnd;
    else
      packet.completeNALU = kNaluIncomplete;
    return packet;
  }

  bool PopPacket(VCMPacket* packet, int index) {
    std::list<VCMPacket>::iterator it = GetPacketIterator(index);
    if (it == packets_.end())
      return false;
    if (packet)
      *packet = (*it);
    packets_.erase(it);
    return true;
  }

  bool GetPacket(VCMPacket* packet, int index) {
    std::list<VCMPacket>::iterator it = GetPacketIterator(index);
    if (it == packets_.end())
      return false;
    if (packet)
      *packet = (*it);
    return true;
  }

  bool NextPacket(VCMPacket* packet) {
    if (packets_.empty())
      return false;
    if (packet != NULL)
      *packet = packets_.front();
    packets_.pop_front();
    return true;
  }

  uint16_t NextSequenceNumber() const {
    if (packets_.empty())
      return sequence_number_;
    return packets_.front().seqNum;
  }

  int PacketsRemaining() const {
    return packets_.size();
  }

 private:
  std::list<VCMPacket>::iterator GetPacketIterator(int index) {
    std::list<VCMPacket>::iterator it = packets_.begin();
    for (int i = 0; i < index; ++i) {
      ++it;
      if (it == packets_.end()) break;
    }
    return it;
  }

  std::list<VCMPacket> packets_;
  uint16_t sequence_number_;
  uint32_t timestamp_;
  int64_t start_time_;

  DISALLOW_COPY_AND_ASSIGN(StreamGenerator);
};

class TestRunningJitterBuffer : public ::testing::Test {
 protected:
  enum { kDataBufferSize = 10 };
  enum { kDefaultFrameRate = 25 };
  enum { kDefaultFramePeriodMs = 1000 / kDefaultFrameRate };

  virtual void SetUp() {
    clock_ = new FakeTickTime(0);
    jitter_buffer_ = new VCMJitterBuffer(clock_);
    stream_generator = new StreamGenerator(0, 0,
                                           clock_->MillisecondTimestamp());
    jitter_buffer_->Start();
    memset(data_buffer_, 0, kDataBufferSize);
  }

  virtual void TearDown() {
    jitter_buffer_->Stop();
    delete stream_generator;
    delete jitter_buffer_;
    delete clock_;
  }

  VCMFrameBufferEnum InsertPacketAndPop(int index) {
    VCMPacket packet;
    VCMEncodedFrame* frame;

    packet.dataPtr = data_buffer_;
    bool packet_available = stream_generator->PopPacket(&packet, index);
    EXPECT_TRUE(packet_available);
    if (!packet_available)
      return kStateError;  
    EXPECT_EQ(VCM_OK, jitter_buffer_->GetFrame(packet, frame));
    return jitter_buffer_->InsertPacket(frame, packet);
  }

  VCMFrameBufferEnum InsertPacket(int index) {
    VCMPacket packet;
    VCMEncodedFrame* frame;

    packet.dataPtr = data_buffer_;
    bool packet_available = stream_generator->GetPacket(&packet, index);
    EXPECT_TRUE(packet_available);
    if (!packet_available)
      return kStateError;  
    EXPECT_EQ(VCM_OK, jitter_buffer_->GetFrame(packet, frame));
    return jitter_buffer_->InsertPacket(frame, packet);
  }

  void InsertFrame(FrameType frame_type) {
    stream_generator->GenerateFrame(frame_type,
                                    (frame_type != kFrameEmpty) ? 1 : 0,
                                    (frame_type == kFrameEmpty) ? 1 : 0,
                                    clock_->MillisecondTimestamp());
    EXPECT_EQ(kFirstPacket, InsertPacketAndPop(0));
    clock_->IncrementDebugClock(kDefaultFramePeriodMs);
  }

  void InsertFrames(int num_frames, FrameType frame_type) {
    for (int i = 0; i < num_frames; ++i) {
      InsertFrame(frame_type);
    }
  }

  void DropFrame(int num_packets) {
    stream_generator->GenerateFrame(kVideoFrameDelta, num_packets, 0,
                                    clock_->MillisecondTimestamp());
    clock_->IncrementDebugClock(kDefaultFramePeriodMs);
  }

  bool DecodeCompleteFrame() {
    VCMEncodedFrame* frame = jitter_buffer_->GetCompleteFrameForDecoding(0);
    bool ret = (frame != NULL);
    jitter_buffer_->ReleaseFrame(frame);
    return ret;
  }

  bool DecodeFrame() {
    VCMEncodedFrame* frame = jitter_buffer_->GetFrameForDecoding();
    bool ret = (frame != NULL);
    jitter_buffer_->ReleaseFrame(frame);
    return ret;
  }

  VCMJitterBuffer* jitter_buffer_;
  StreamGenerator* stream_generator;
  FakeTickTime* clock_;
  uint8_t data_buffer_[kDataBufferSize];
};

class TestJitterBufferNack : public TestRunningJitterBuffer {
 protected:
  virtual void SetUp() {
    TestRunningJitterBuffer::SetUp();
    jitter_buffer_->SetNackMode(kNackInfinite, -1, -1);
  }

  virtual void TearDown() {
    TestRunningJitterBuffer::TearDown();
  }
};

TEST_F(TestRunningJitterBuffer, TestFull) {
  
  InsertFrame(kVideoFrameKey);
  EXPECT_TRUE(DecodeCompleteFrame());
  DropFrame(1);
  
  InsertFrames(kMaxNumberOfFrames, kVideoFrameDelta);
  
  EXPECT_FALSE(DecodeCompleteFrame());
  
  
  
  InsertFrame(kVideoFrameDelta);
  EXPECT_FALSE(DecodeCompleteFrame());
}

TEST_F(TestRunningJitterBuffer, TestEmptyPackets) {
  
  stream_generator->GenerateFrame(kVideoFrameKey, 3, 3,
                                  clock_->MillisecondTimestamp());
  EXPECT_EQ(kFirstPacket, InsertPacketAndPop(4));
  EXPECT_EQ(kIncomplete, InsertPacketAndPop(4));
  EXPECT_EQ(kIncomplete, InsertPacketAndPop(0));
  EXPECT_EQ(kIncomplete, InsertPacketAndPop(0));
  EXPECT_EQ(kCompleteSession, InsertPacketAndPop(0));
}

TEST_F(TestJitterBufferNack, TestEmptyPackets) {
  
  jitter_buffer_->SetNackMode(kNackHybrid, kLowRttNackMs, -1);
  InsertFrames(kMaxNumberOfFrames, kFrameEmpty);
  InsertFrame(kVideoFrameKey);
  EXPECT_TRUE(DecodeCompleteFrame());
}

TEST_F(TestJitterBufferNack, TestNackListFull) {
  
  InsertFrame(kVideoFrameKey);
  EXPECT_TRUE(DecodeCompleteFrame());

  
  DropFrame(kNackHistoryLength);
  
  InsertFrame(kVideoFrameDelta);
  EXPECT_FALSE(DecodeCompleteFrame());

  uint16_t nack_list_length = kNackHistoryLength;
  bool extended;
  uint16_t* nack_list = jitter_buffer_->CreateNackList(&nack_list_length,
                                                       &extended);
  
  EXPECT_TRUE(nack_list_length == 0xffff && nack_list == NULL);

  InsertFrame(kVideoFrameDelta);
  EXPECT_FALSE(DecodeCompleteFrame());
  EXPECT_FALSE(DecodeFrame());
}

TEST_F(TestJitterBufferNack, TestNackBeforeDecode) {
  DropFrame(10);
  
  InsertFrame(kVideoFrameDelta);
  uint16_t nack_list_size = 0;
  bool extended = false;
  uint16_t* list = jitter_buffer_->CreateNackList(&nack_list_size, &extended);
  
  EXPECT_TRUE(list == NULL);
  EXPECT_EQ(0xFFFF, nack_list_size);
}

TEST_F(TestJitterBufferNack, TestNormalOperation) {
  EXPECT_EQ(kNackInfinite, jitter_buffer_->nack_mode());

  InsertFrame(kVideoFrameKey);
  EXPECT_TRUE(DecodeFrame());

  
  
  
  stream_generator->GenerateFrame(kVideoFrameKey, 100, 0,
                                  clock_->MillisecondTimestamp());
  clock_->IncrementDebugClock(kDefaultFramePeriodMs);
  EXPECT_EQ(kFirstPacket, InsertPacketAndPop(0));
  
  EXPECT_FALSE(DecodeCompleteFrame());
  while (stream_generator->PacketsRemaining() > 1) {
    if (stream_generator->NextSequenceNumber() % 10 != 0)
      EXPECT_EQ(kIncomplete, InsertPacketAndPop(0));
    else
      stream_generator->NextPacket(NULL);  
  }
  EXPECT_EQ(kIncomplete, InsertPacketAndPop(0));
  EXPECT_EQ(0, stream_generator->PacketsRemaining());
  EXPECT_FALSE(DecodeCompleteFrame());
  EXPECT_FALSE(DecodeFrame());
  uint16_t nack_list_size = 0;
  bool extended = false;
  uint16_t* list = jitter_buffer_->CreateNackList(&nack_list_size, &extended);
  
  const int kExpectedNackSize = 9;
  ASSERT_EQ(kExpectedNackSize, nack_list_size);
  for (int i = 0; i < nack_list_size; ++i)
    EXPECT_EQ((1 + i) * 10, list[i]);
}

TEST_F(TestJitterBufferNack, TestNormalOperationWrap) {
  
  
  
  stream_generator->Init(65532, 0, clock_->MillisecondTimestamp());
  InsertFrame(kVideoFrameKey);
  EXPECT_TRUE(DecodeCompleteFrame());
  stream_generator->GenerateFrame(kVideoFrameDelta, 100, 0,
                                  clock_->MillisecondTimestamp());
  EXPECT_EQ(kFirstPacket, InsertPacketAndPop(0));
  while (stream_generator->PacketsRemaining() > 1) {
    if (stream_generator->NextSequenceNumber() % 10 != 0)
      EXPECT_EQ(kIncomplete, InsertPacketAndPop(0));
    else
      stream_generator->NextPacket(NULL);  
  }
  EXPECT_EQ(kIncomplete, InsertPacketAndPop(0));
  EXPECT_EQ(0, stream_generator->PacketsRemaining());
  EXPECT_FALSE(DecodeCompleteFrame());
  EXPECT_FALSE(DecodeCompleteFrame());
  uint16_t nack_list_size = 0;
  bool extended = false;
  uint16_t* list = jitter_buffer_->CreateNackList(&nack_list_size, &extended);
  
  const int kExpectedNackSize = 10;
  ASSERT_EQ(kExpectedNackSize, nack_list_size);
  for (int i = 0; i < nack_list_size; ++i)
    EXPECT_EQ(i * 10, list[i]);
}

}  
