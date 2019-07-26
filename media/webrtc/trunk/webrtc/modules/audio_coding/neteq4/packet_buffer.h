









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_PACKET_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_PACKET_BUFFER_H_

#include "webrtc/modules/audio_coding/neteq4/packet.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class DecoderDatabase;


class PacketBuffer {
 public:
  enum BufferReturnCodes {
    kOK = 0,
    kFlushed,
    kNotFound,
    kBufferEmpty,
    kInvalidPacket,
    kInvalidPointer,
    kOversizePacket
  };

  
  
  
  PacketBuffer(size_t max_number_of_packets, size_t max_payload_memory);

  
  virtual ~PacketBuffer();

  
  virtual void Flush();

  
  virtual bool Empty() const { return buffer_.empty(); }

  
  
  
  
  virtual int InsertPacket(Packet* packet);

  
  
  
  
  
  
  
  
  virtual int InsertPacketList(PacketList* packet_list,
                               const DecoderDatabase& decoder_database,
                               uint8_t* current_rtp_payload_type,
                               uint8_t* current_cng_rtp_payload_type);

  
  
  
  
  virtual int NextTimestamp(uint32_t* next_timestamp) const;

  
  
  
  
  
  virtual int NextHigherTimestamp(uint32_t timestamp,
                                  uint32_t* next_timestamp) const;

  
  
  virtual const RTPHeader* NextRtpHeader() const;

  
  
  
  
  
  
  virtual Packet* GetNextPacket(int* discard_count);

  
  
  
  virtual int DiscardNextPacket();

  
  
  virtual int DiscardOldPackets(uint32_t timestamp_limit);

  
  
  virtual int NumPacketsInBuffer() const {
    return static_cast<int>(buffer_.size());
  }

  
  
  virtual int NumSamplesInBuffer(DecoderDatabase* decoder_database,
                                 int last_decoded_length) const;

  
  
  virtual void IncrementWaitingTimes(int inc = 1);

  virtual void BufferStat(int* num_packets,
                          int* max_num_packets,
                          int* current_memory_bytes,
                          int* max_memory_bytes) const;

  virtual int current_memory_bytes() const { return current_memory_bytes_; }

  
  
  
  static bool DeleteFirstPacket(PacketList* packet_list);

  
  
  static void DeleteAllPackets(PacketList* packet_list);

 private:
  size_t max_number_of_packets_;
  size_t max_memory_bytes_;
  int current_memory_bytes_;
  PacketList buffer_;
  DISALLOW_COPY_AND_ASSIGN(PacketBuffer);
};

}  
#endif  
