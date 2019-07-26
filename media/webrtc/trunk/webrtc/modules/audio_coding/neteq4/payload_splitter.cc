









#include "webrtc/modules/audio_coding/neteq4/payload_splitter.h"

#include <assert.h>

#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"

namespace webrtc {








int PayloadSplitter::SplitRed(PacketList* packet_list) {
  int ret = kOK;
  PacketList::iterator it = packet_list->begin();
  while (it != packet_list->end()) {
    PacketList new_packets;  
    Packet* red_packet = (*it);
    assert(red_packet->payload);
    uint8_t* payload_ptr = red_packet->payload;

    
    
    
    
    
    
    
    
    
    
    
    

    bool last_block = false;
    int sum_length = 0;
    while (!last_block) {
      Packet* new_packet = new Packet;
      new_packet->header = red_packet->header;
      
      last_block = ((*payload_ptr & 0x80) == 0);
      
      new_packet->header.payloadType = payload_ptr[0] & 0x7F;
      if (last_block) {
        
        ++sum_length;  
        new_packet->payload_length = red_packet->payload_length - sum_length;
        new_packet->primary = true;  
        payload_ptr += 1;  
      } else {
        
        int timestamp_offset = (payload_ptr[1] << 6) +
            ((payload_ptr[2] & 0xFC) >> 2);
        new_packet->header.timestamp = red_packet->header.timestamp -
            timestamp_offset;
        
        new_packet->payload_length = ((payload_ptr[2] & 0x03) << 8) +
            payload_ptr[3];
        new_packet->primary = false;
        payload_ptr += 4;  
      }
      sum_length += new_packet->payload_length;
      sum_length += 4;  
      
      new_packets.push_back(new_packet);
    }

    
    
    PacketList::iterator new_it;
    for (new_it = new_packets.begin(); new_it != new_packets.end(); ++new_it) {
      int payload_length = (*new_it)->payload_length;
      if (payload_ptr + payload_length >
          red_packet->payload + red_packet->payload_length) {
        
        
        
        while (new_it != new_packets.end()) {
          
          assert(!(*new_it)->payload);
          delete (*new_it);
          new_it = new_packets.erase(new_it);
        }
        ret = kRedLengthMismatch;
        break;
      }
      (*new_it)->payload = new uint8_t[payload_length];
      memcpy((*new_it)->payload, payload_ptr, payload_length);
      payload_ptr += payload_length;
    }
    
    
    new_packets.reverse();
    
    
    packet_list->splice(it, new_packets, new_packets.begin(),
                        new_packets.end());
    
    delete [] (*it)->payload;
    delete (*it);
    
    
    
    it = packet_list->erase(it);
  }
  return ret;
}

int PayloadSplitter::CheckRedPayloads(PacketList* packet_list,
                                      const DecoderDatabase& decoder_database) {
  PacketList::iterator it = packet_list->begin();
  int main_payload_type = -1;
  int num_deleted_packets = 0;
  while (it != packet_list->end()) {
    uint8_t this_payload_type = (*it)->header.payloadType;
    if (!decoder_database.IsDtmf(this_payload_type) &&
        !decoder_database.IsComfortNoise(this_payload_type)) {
      if (main_payload_type == -1) {
        
        main_payload_type = this_payload_type;
      } else {
        if (this_payload_type != main_payload_type) {
          
          
          delete [] (*it)->payload;
          delete (*it);
          
          
          
          it = packet_list->erase(it);
          ++num_deleted_packets;
          continue;
        }
      }
    }
    ++it;
  }
  return num_deleted_packets;
}

int PayloadSplitter::SplitAudio(PacketList* packet_list,
                                const DecoderDatabase& decoder_database) {
  PacketList::iterator it = packet_list->begin();
  
  while (it != packet_list->end()) {
    Packet* packet = (*it);  
    
    const DecoderDatabase::DecoderInfo* info =
        decoder_database.GetDecoderInfo(packet->header.payloadType);
    if (!info) {
      return kUnknownPayloadType;
    }
    
    if (packet->sync_packet) {
      ++it;
      continue;
    }
    PacketList new_packets;
    switch (info->codec_type) {
      case kDecoderPCMu:
      case kDecoderPCMa: {
        
        SplitBySamples(packet, 8, 8, &new_packets);
        break;
      }
      case kDecoderPCMu_2ch:
      case kDecoderPCMa_2ch: {
        
        SplitBySamples(packet, 2 * 8, 8, &new_packets);
        break;
      }
      case kDecoderG722: {
        
        SplitBySamples(packet, 8, 16, &new_packets);
        break;
      }
      case kDecoderPCM16B: {
        
        SplitBySamples(packet, 16, 8, &new_packets);
        break;
      }
      case kDecoderPCM16Bwb: {
        
        SplitBySamples(packet, 32, 16, &new_packets);
        break;
      }
      case kDecoderPCM16Bswb32kHz: {
        
        SplitBySamples(packet, 64, 32, &new_packets);
        break;
      }
      case kDecoderPCM16Bswb48kHz: {
        
        SplitBySamples(packet, 96, 48, &new_packets);
        break;
      }
      case kDecoderPCM16B_2ch: {
        
        SplitBySamples(packet, 2 * 16, 8, &new_packets);
        break;
      }
      case kDecoderPCM16Bwb_2ch: {
        
        SplitBySamples(packet, 2 * 32, 16, &new_packets);
        break;
      }
      case kDecoderPCM16Bswb32kHz_2ch: {
        
        SplitBySamples(packet, 2 * 64, 32, &new_packets);
        break;
      }
      case kDecoderPCM16Bswb48kHz_2ch: {
        
        SplitBySamples(packet, 2 * 96, 48, &new_packets);
        break;
      }
      case kDecoderPCM16B_5ch: {
        
        SplitBySamples(packet, 5 * 16, 8, &new_packets);
        break;
      }
      case kDecoderILBC: {
        int bytes_per_frame;
        int timestamps_per_frame;
        if (packet->payload_length >= 950) {
          return kTooLargePayload;
        } else if (packet->payload_length % 38 == 0) {
          
          bytes_per_frame = 38;
          timestamps_per_frame = 160;
        } else if (packet->payload_length % 50 == 0) {
          
          bytes_per_frame = 50;
          timestamps_per_frame = 240;
        } else {
          return kFrameSplitError;
        }
        int ret = SplitByFrames(packet, bytes_per_frame, timestamps_per_frame,
                                &new_packets);
        if (ret < 0) {
          return ret;
        } else if (ret == kNoSplit) {
          
          ++it;
          
          
          
          continue;
        }
        break;
      }
      default: {
        
        ++it;
        
        
        
        continue;
      }
    }
    
    
    packet_list->splice(it, new_packets, new_packets.begin(),
                        new_packets.end());
    
    delete [] (*it)->payload;
    delete (*it);
    
    
    
    it = packet_list->erase(it);
  }
  return 0;
}

void PayloadSplitter::SplitBySamples(const Packet* packet,
                                     int bytes_per_ms,
                                     int timestamps_per_ms,
                                     PacketList* new_packets) {
  assert(packet);
  assert(new_packets);

  int split_size_bytes = packet->payload_length;

  
  int min_chunk_size = bytes_per_ms * 20;
  
  
  
  while (split_size_bytes >= 2 * min_chunk_size) {
    split_size_bytes >>= 1;
  }
  int timestamps_per_chunk =
      split_size_bytes * timestamps_per_ms / bytes_per_ms;
  uint32_t timestamp = packet->header.timestamp;

  uint8_t* payload_ptr = packet->payload;
  int len = packet->payload_length;
  while (len >= (2 * split_size_bytes)) {
    Packet* new_packet = new Packet;
    new_packet->payload_length = split_size_bytes;
    new_packet->header = packet->header;
    new_packet->header.timestamp = timestamp;
    timestamp += timestamps_per_chunk;
    new_packet->primary = packet->primary;
    new_packet->payload = new uint8_t[split_size_bytes];
    memcpy(new_packet->payload, payload_ptr, split_size_bytes);
    payload_ptr += split_size_bytes;
    new_packets->push_back(new_packet);
    len -= split_size_bytes;
  }

  if (len > 0) {
    Packet* new_packet = new Packet;
    new_packet->payload_length = len;
    new_packet->header = packet->header;
    new_packet->header.timestamp = timestamp;
    new_packet->primary = packet->primary;
    new_packet->payload = new uint8_t[len];
    memcpy(new_packet->payload, payload_ptr, len);
    new_packets->push_back(new_packet);
  }
}

int PayloadSplitter::SplitByFrames(const Packet* packet,
                                   int bytes_per_frame,
                                   int timestamps_per_frame,
                                   PacketList* new_packets) {
  if (packet->payload_length % bytes_per_frame != 0) {
    return kFrameSplitError;
  }

  int num_frames = packet->payload_length / bytes_per_frame;
  if (num_frames == 1) {
    
    return kNoSplit;
  }

  uint32_t timestamp = packet->header.timestamp;
  uint8_t* payload_ptr = packet->payload;
  int len = packet->payload_length;
  while (len > 0) {
    assert(len >= bytes_per_frame);
    Packet* new_packet = new Packet;
    new_packet->payload_length = bytes_per_frame;
    new_packet->header = packet->header;
    new_packet->header.timestamp = timestamp;
    timestamp += timestamps_per_frame;
    new_packet->primary = packet->primary;
    new_packet->payload = new uint8_t[bytes_per_frame];
    memcpy(new_packet->payload, payload_ptr, bytes_per_frame);
    payload_ptr += bytes_per_frame;
    new_packets->push_back(new_packet);
    len -= bytes_per_frame;
  }
  return kOK;
}

}  
