









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_PACKET_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_PACKET_BUFFER_H_

#include "webrtc/modules/audio_coding/neteq4/packet_buffer.h"

#include "gmock/gmock.h"

namespace webrtc {

class MockPacketBuffer : public PacketBuffer {
 public:
  MockPacketBuffer(size_t max_number_of_packets, size_t max_payload_memory)
      : PacketBuffer(max_number_of_packets, max_payload_memory) {}
  virtual ~MockPacketBuffer() { Die(); }
  MOCK_METHOD0(Die, void());
  MOCK_METHOD0(Flush,
      void());
  MOCK_CONST_METHOD0(Empty,
      bool());
  MOCK_METHOD1(InsertPacket,
      int(Packet* packet));
  MOCK_METHOD4(InsertPacketList,
      int(PacketList* packet_list,
          const DecoderDatabase& decoder_database,
          uint8_t* current_rtp_payload_type,
          uint8_t* current_cng_rtp_payload_type));
  MOCK_CONST_METHOD1(NextTimestamp,
      int(uint32_t* next_timestamp));
  MOCK_CONST_METHOD2(NextHigherTimestamp,
      int(uint32_t timestamp, uint32_t* next_timestamp));
  MOCK_CONST_METHOD0(NextRtpHeader,
      const RTPHeader*());
  MOCK_METHOD1(GetNextPacket,
      Packet*(int* discard_count));
  MOCK_METHOD0(DiscardNextPacket,
      int());
  MOCK_METHOD1(DiscardOldPackets,
      int(uint32_t timestamp_limit));
  MOCK_CONST_METHOD0(NumPacketsInBuffer,
      int());
  MOCK_METHOD1(IncrementWaitingTimes,
      void(int));
  MOCK_CONST_METHOD0(current_memory_bytes,
      int());
};

}  
#endif  
