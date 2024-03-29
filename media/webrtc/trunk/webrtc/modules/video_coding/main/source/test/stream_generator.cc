









#include "webrtc/modules/video_coding/main/source/test/stream_generator.h"

#include <string.h>

#include <list>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/video_coding/main/source/packet.h"
#include "webrtc/modules/video_coding/main/test/test_util.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {

StreamGenerator::StreamGenerator(uint16_t start_seq_num,
                                 uint32_t start_timestamp,
                                 int64_t current_time)
    : packets_(),
      sequence_number_(start_seq_num),
      timestamp_(start_timestamp),
      start_time_(current_time) {}

void StreamGenerator::Init(uint16_t start_seq_num,
                           uint32_t start_timestamp,
                           int64_t current_time) {
  packets_.clear();
  sequence_number_ = start_seq_num;
  timestamp_ = start_timestamp;
  start_time_ = current_time;
  memset(&packet_buffer, 0, sizeof(packet_buffer));
}

void StreamGenerator::GenerateFrame(FrameType type,
                                    int num_media_packets,
                                    int num_empty_packets,
                                    int64_t current_time) {
  timestamp_ = 90 * (current_time - start_time_);
  for (int i = 0; i < num_media_packets; ++i) {
    const int packet_size =
        (kFrameSize + num_media_packets / 2) / num_media_packets;
    bool marker_bit = (i == num_media_packets - 1);
    packets_.push_back(GeneratePacket(
        sequence_number_, timestamp_, packet_size, (i == 0), marker_bit, type));
    ++sequence_number_;
  }
  for (int i = 0; i < num_empty_packets; ++i) {
    packets_.push_back(GeneratePacket(
        sequence_number_, timestamp_, 0, false, false, kFrameEmpty));
    ++sequence_number_;
  }
}

VCMPacket StreamGenerator::GeneratePacket(uint16_t sequence_number,
                                          uint32_t timestamp,
                                          unsigned int size,
                                          bool first_packet,
                                          bool marker_bit,
                                          FrameType type) {
  EXPECT_LT(size, kMaxPacketSize);
  VCMPacket packet;
  packet.seqNum = sequence_number;
  packet.timestamp = timestamp;
  packet.frameType = type;
  packet.isFirstPacket = first_packet;
  packet.markerBit = marker_bit;
  packet.sizeBytes = size;
  packet.dataPtr = packet_buffer;
  if (packet.isFirstPacket)
    packet.completeNALU = kNaluStart;
  else if (packet.markerBit)
    packet.completeNALU = kNaluEnd;
  else
    packet.completeNALU = kNaluIncomplete;
  return packet;
}

bool StreamGenerator::PopPacket(VCMPacket* packet, int index) {
  std::list<VCMPacket>::iterator it = GetPacketIterator(index);
  if (it == packets_.end())
    return false;
  if (packet)
    *packet = (*it);
  packets_.erase(it);
  return true;
}

bool StreamGenerator::GetPacket(VCMPacket* packet, int index) {
  std::list<VCMPacket>::iterator it = GetPacketIterator(index);
  if (it == packets_.end())
    return false;
  if (packet)
    *packet = (*it);
  return true;
}

bool StreamGenerator::NextPacket(VCMPacket* packet) {
  if (packets_.empty())
    return false;
  if (packet != NULL)
    *packet = packets_.front();
  packets_.pop_front();
  return true;
}

void StreamGenerator::DropLastPacket() { packets_.pop_back(); }

uint16_t StreamGenerator::NextSequenceNumber() const {
  if (packets_.empty())
    return sequence_number_;
  return packets_.front().seqNum;
}

int StreamGenerator::PacketsRemaining() const { return packets_.size(); }

std::list<VCMPacket>::iterator StreamGenerator::GetPacketIterator(int index) {
  std::list<VCMPacket>::iterator it = packets_.begin();
  for (int i = 0; i < index; ++i) {
    ++it;
    if (it == packets_.end())
      break;
  }
  return it;
}

}  
