













#include "webrtc/modules/audio_coding/neteq4/packet_buffer.h"

#include <algorithm>  

#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/interface/audio_decoder.h"

namespace webrtc {



class NewTimestampIsLarger {
 public:
  explicit NewTimestampIsLarger(const Packet* new_packet)
      : new_packet_(new_packet) {
  }
  bool operator()(Packet* packet) {
    return (*new_packet_ >= *packet);
  }

 private:
  const Packet* new_packet_;
};



PacketBuffer::PacketBuffer(size_t max_number_of_packets,
                           size_t max_memory_bytes)
    : max_number_of_packets_(max_number_of_packets),
      max_memory_bytes_(max_memory_bytes),
      current_memory_bytes_(0) {
}


PacketBuffer::~PacketBuffer() {
  Flush();
}


void PacketBuffer::Flush() {
  DeleteAllPackets(&buffer_);
  current_memory_bytes_ = 0;
}

int PacketBuffer::InsertPacket(Packet* packet) {
  if (!packet || !packet->payload) {
    if (packet) {
      delete packet;
    }
    return kInvalidPacket;
  }

  int return_val = kOK;

  if ((buffer_.size() >= max_number_of_packets_) ||
      (current_memory_bytes_ + packet->payload_length
          > static_cast<int>(max_memory_bytes_))) {
    
    Flush();
    return_val = kFlushed;
    if ((buffer_.size() >= max_number_of_packets_) ||
        (current_memory_bytes_ + packet->payload_length
            > static_cast<int>(max_memory_bytes_))) {
      
      
      
      delete [] packet->payload;
      delete packet;
      return kOversizePacket;
    }
  }

  
  
  
  PacketList::reverse_iterator rit = std::find_if(
      buffer_.rbegin(), buffer_.rend(),
      NewTimestampIsLarger(packet));
  buffer_.insert(rit.base(), packet);  
  current_memory_bytes_ += packet->payload_length;

  return return_val;
}

int PacketBuffer::InsertPacketList(PacketList* packet_list,
                                   const DecoderDatabase& decoder_database,
                                   uint8_t* current_rtp_payload_type,
                                   uint8_t* current_cng_rtp_payload_type) {
  bool flushed = false;
  while (!packet_list->empty()) {
    Packet* packet = packet_list->front();
    if (decoder_database.IsComfortNoise(packet->header.payloadType)) {
      if (*current_cng_rtp_payload_type != 0xFF &&
          *current_cng_rtp_payload_type != packet->header.payloadType) {
        
        *current_rtp_payload_type = 0xFF;
        Flush();
        flushed = true;
      }
      *current_cng_rtp_payload_type = packet->header.payloadType;
    } else if (!decoder_database.IsDtmf(packet->header.payloadType)) {
      
      if (*current_rtp_payload_type != 0xFF &&
          *current_rtp_payload_type != packet->header.payloadType) {
        *current_cng_rtp_payload_type = 0xFF;
        Flush();
        flushed = true;
      }
      *current_rtp_payload_type = packet->header.payloadType;
    }
    int return_val = InsertPacket(packet);
    packet_list->pop_front();
    if (return_val == kFlushed) {
      
      flushed = true;
    } else if (return_val != kOK) {
      
      DeleteAllPackets(packet_list);
      return return_val;
    }
  }
  return flushed ? kFlushed : kOK;
}

int PacketBuffer::NextTimestamp(uint32_t* next_timestamp) const {
  if (Empty()) {
    return kBufferEmpty;
  }
  if (!next_timestamp) {
    return kInvalidPointer;
  }
  *next_timestamp = buffer_.front()->header.timestamp;
  return kOK;
}

int PacketBuffer::NextHigherTimestamp(uint32_t timestamp,
                                      uint32_t* next_timestamp) const {
  if (Empty()) {
    return kBufferEmpty;
  }
  if (!next_timestamp) {
    return kInvalidPointer;
  }
  PacketList::const_iterator it;
  for (it = buffer_.begin(); it != buffer_.end(); ++it) {
    if ((*it)->header.timestamp >= timestamp) {
      
      *next_timestamp = (*it)->header.timestamp;
      return kOK;
    }
  }
  return kNotFound;
}

const RTPHeader* PacketBuffer::NextRtpHeader() const {
  if (Empty()) {
    return NULL;
  }
  return const_cast<const RTPHeader*>(&(buffer_.front()->header));
}

Packet* PacketBuffer::GetNextPacket(int* discard_count) {
  if (Empty()) {
    
    return NULL;
  }

  Packet* packet = buffer_.front();
  
  assert(packet && packet->payload);
  buffer_.pop_front();
  current_memory_bytes_ -= packet->payload_length;
  assert(current_memory_bytes_ >= 0);  
  
  
  if (discard_count) {
    *discard_count = 0;
  }
  while (!Empty() &&
      buffer_.front()->header.timestamp == packet->header.timestamp) {
    if (DiscardNextPacket() != kOK) {
      assert(false);  
    }
    if (discard_count) {
      ++(*discard_count);
    }
  }
  return packet;
}

int PacketBuffer::DiscardNextPacket() {
  if (Empty()) {
    return kBufferEmpty;
  }
  Packet* temp_packet = buffer_.front();
  
  assert(temp_packet && temp_packet->payload);
  current_memory_bytes_ -= temp_packet->payload_length;
  assert(current_memory_bytes_ >= 0);  
  DeleteFirstPacket(&buffer_);
  return kOK;
}

int PacketBuffer::DiscardOldPackets(uint32_t timestamp_limit) {
  int discard_count = 0;
  while (!Empty() &&
      timestamp_limit != buffer_.front()->header.timestamp &&
      static_cast<uint32_t>(timestamp_limit
                            - buffer_.front()->header.timestamp) <
                            0xFFFFFFFF / 2) {
    if (DiscardNextPacket() != kOK) {
      assert(false);  
    }
    ++discard_count;
  }
  return 0;
}

int PacketBuffer::NumSamplesInBuffer(DecoderDatabase* decoder_database,
                                     int last_decoded_length) const {
  PacketList::const_iterator it;
  int num_samples = 0;
  int last_duration = last_decoded_length;
  for (it = buffer_.begin(); it != buffer_.end(); ++it) {
    Packet* packet = (*it);
    AudioDecoder* decoder =
        decoder_database->GetDecoder(packet->header.payloadType);
    if (decoder) {
      int duration = packet->sync_packet ? last_duration :
          decoder->PacketDuration(packet->payload, packet->payload_length);
      if (duration >= 0) {
        last_duration = duration;  
      }
    }
    num_samples += last_duration;
  }
  return num_samples;
}

void PacketBuffer::IncrementWaitingTimes(int inc) {
  PacketList::iterator it;
  for (it = buffer_.begin(); it != buffer_.end(); ++it) {
    (*it)->waiting_time += inc;
  }
}

bool PacketBuffer::DeleteFirstPacket(PacketList* packet_list) {
  if (packet_list->empty()) {
    return false;
  }
  Packet* first_packet = packet_list->front();
  delete [] first_packet->payload;
  delete first_packet;
  packet_list->pop_front();
  return true;
}

void PacketBuffer::DeleteAllPackets(PacketList* packet_list) {
  while (DeleteFirstPacket(packet_list)) {
    
  }
}

void PacketBuffer::BufferStat(int* num_packets,
                              int* max_num_packets,
                              int* current_memory_bytes,
                              int* max_memory_bytes) const {
  *num_packets = static_cast<int>(buffer_.size());
  *max_num_packets = static_cast<int>(max_number_of_packets_);
  *current_memory_bytes = current_memory_bytes_;
  *max_memory_bytes = static_cast<int>(max_memory_bytes_);
}

}  
