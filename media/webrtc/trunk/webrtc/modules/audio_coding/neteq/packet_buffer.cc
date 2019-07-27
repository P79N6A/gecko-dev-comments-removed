













#include "webrtc/modules/audio_coding/neteq/packet_buffer.h"

#include <algorithm>  

#include "webrtc/modules/audio_coding/neteq/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq/interface/audio_decoder.h"

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

PacketBuffer::PacketBuffer(size_t max_number_of_packets)
    : max_number_of_packets_(max_number_of_packets) {}


PacketBuffer::~PacketBuffer() {
  Flush();
}


void PacketBuffer::Flush() {
  DeleteAllPackets(&buffer_);
}

int PacketBuffer::InsertPacket(Packet* packet) {
  if (!packet || !packet->payload) {
    if (packet) {
      delete packet;
    }
    return kInvalidPacket;
  }

  int return_val = kOK;

  if (buffer_.size() >= max_number_of_packets_) {
    
    Flush();
    return_val = kFlushed;
  }

  
  
  
  PacketList::reverse_iterator rit = std::find_if(
      buffer_.rbegin(), buffer_.rend(),
      NewTimestampIsLarger(packet));

  
  
  
  if (rit != buffer_.rend() &&
      packet->header.timestamp == (*rit)->header.timestamp) {
    delete [] packet->payload;
    delete packet;
    return return_val;
  }

  
  
  
  PacketList::iterator it = rit.base();
  if (it != buffer_.end() &&
      packet->header.timestamp == (*it)->header.timestamp) {
    delete [] (*it)->payload;
    delete *it;
    it = buffer_.erase(it);
  }
  buffer_.insert(it, packet);  

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

  
  
  int discards = 0;

  while (!Empty() &&
      buffer_.front()->header.timestamp == packet->header.timestamp) {
    if (DiscardNextPacket() != kOK) {
      assert(false);  
    }
    ++discards;
  }
  
  
  assert(discards == 0);
  if (discard_count)
    *discard_count = discards;

  return packet;
}

int PacketBuffer::DiscardNextPacket() {
  if (Empty()) {
    return kBufferEmpty;
  }
  
  assert(buffer_.front());
  assert(buffer_.front()->payload);
  DeleteFirstPacket(&buffer_);
  return kOK;
}

int PacketBuffer::DiscardOldPackets(uint32_t timestamp_limit,
                                    uint32_t horizon_samples) {
  while (!Empty() && timestamp_limit != buffer_.front()->header.timestamp &&
         IsObsoleteTimestamp(buffer_.front()->header.timestamp,
                             timestamp_limit,
                             horizon_samples)) {
    if (DiscardNextPacket() != kOK) {
      assert(false);  
    }
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
      int duration;
      if (packet->sync_packet) {
        duration = last_duration;
      } else if (packet->primary) {
        duration =
            decoder->PacketDuration(packet->payload, packet->payload_length);
      } else {
        continue;
      }
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

void PacketBuffer::BufferStat(int* num_packets, int* max_num_packets) const {
  *num_packets = static_cast<int>(buffer_.size());
  *max_num_packets = static_cast<int>(max_number_of_packets_);
}

}  
