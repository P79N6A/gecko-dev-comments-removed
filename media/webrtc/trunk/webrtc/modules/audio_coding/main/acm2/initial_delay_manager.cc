









#include "webrtc/modules/audio_coding/main/acm2/initial_delay_manager.h"

namespace webrtc {

namespace acm2 {

InitialDelayManager::InitialDelayManager(int initial_delay_ms,
                                         int late_packet_threshold)
    : last_packet_type_(kUndefinedPacket),
      last_receive_timestamp_(0),
      timestamp_step_(0),
      audio_payload_type_(kInvalidPayloadType),
      initial_delay_ms_(initial_delay_ms),
      buffered_audio_ms_(0),
      buffering_(true),
      playout_timestamp_(0),
      late_packet_threshold_(late_packet_threshold) {
  last_packet_rtp_info_.header.payloadType = kInvalidPayloadType;
  last_packet_rtp_info_.header.ssrc = 0;
  last_packet_rtp_info_.header.sequenceNumber = 0;
  last_packet_rtp_info_.header.timestamp = 0;
}

void InitialDelayManager::UpdateLastReceivedPacket(
    const WebRtcRTPHeader& rtp_info,
    uint32_t receive_timestamp,
    PacketType type,
    bool new_codec,
    int sample_rate_hz,
    SyncStream* sync_stream) {
  assert(sync_stream);

  
  assert(!(!new_codec && type == kAudioPacket &&
         rtp_info.header.payloadType != audio_payload_type_));

  
  const RTPHeader* current_header = &rtp_info.header;
  RTPHeader* last_header = &last_packet_rtp_info_.header;

  
  
  
  
  if (type == kAvtPacket ||
      (last_packet_type_ != kUndefinedPacket &&
      !IsNewerSequenceNumber(current_header->sequenceNumber,
                             last_header->sequenceNumber))) {
    sync_stream->num_sync_packets = 0;
    return;
  }

  
  if (new_codec ||
      last_packet_rtp_info_.header.payloadType == kInvalidPayloadType) {
    timestamp_step_ = 0;
    if (type == kAudioPacket)
      audio_payload_type_ = rtp_info.header.payloadType;
    else
      audio_payload_type_ = kInvalidPayloadType;  

    RecordLastPacket(rtp_info, receive_timestamp, type);
    sync_stream->num_sync_packets = 0;
    buffered_audio_ms_ = 0;
    buffering_ = true;

    
    
    UpdatePlayoutTimestamp(*current_header, sample_rate_hz);
    return;
  }

  uint32_t timestamp_increase = current_header->timestamp -
      last_header->timestamp;

  
  
  if (last_packet_type_ == kUndefinedPacket) {
    timestamp_increase = 0;
  }

  if (buffering_) {
    buffered_audio_ms_ += timestamp_increase * 1000 / sample_rate_hz;

    
    UpdatePlayoutTimestamp(*current_header, sample_rate_hz);

    if (buffered_audio_ms_ >= initial_delay_ms_)
      buffering_ = false;
  }

  if (current_header->sequenceNumber == last_header->sequenceNumber + 1) {
    
    
    if (last_packet_type_ == kAudioPacket)
      timestamp_step_ = timestamp_increase;
    RecordLastPacket(rtp_info, receive_timestamp, type);
    sync_stream->num_sync_packets = 0;
    return;
  }

  uint16_t packet_gap = current_header->sequenceNumber -
      last_header->sequenceNumber - 1;

  
  sync_stream->num_sync_packets = last_packet_type_ == kSyncPacket ?
      packet_gap - 1 : packet_gap - 2;

  
  if (sync_stream->num_sync_packets > 0 &&
      audio_payload_type_ != kInvalidPayloadType) {
    if (timestamp_step_ == 0) {
      
      assert(packet_gap > 0);
      timestamp_step_ = timestamp_increase / (packet_gap + 1);
    }
    sync_stream->timestamp_step = timestamp_step_;

    
    memcpy(&sync_stream->rtp_info, &rtp_info, sizeof(rtp_info));
    sync_stream->rtp_info.header.payloadType = audio_payload_type_;

    uint16_t sequence_number_update = sync_stream->num_sync_packets + 1;
    uint32_t timestamp_update = timestamp_step_ * sequence_number_update;

    
    
    
    
    
    
    sync_stream->rtp_info.header.sequenceNumber -= sequence_number_update;
    sync_stream->receive_timestamp = receive_timestamp - timestamp_update;
    sync_stream->rtp_info.header.timestamp -= timestamp_update;
    sync_stream->rtp_info.header.payloadType = audio_payload_type_;
  } else {
    sync_stream->num_sync_packets = 0;
  }

  RecordLastPacket(rtp_info, receive_timestamp, type);
  return;
}

void InitialDelayManager::RecordLastPacket(const WebRtcRTPHeader& rtp_info,
                                           uint32_t receive_timestamp,
                                           PacketType type) {
  last_packet_type_ = type;
  last_receive_timestamp_ = receive_timestamp;
  memcpy(&last_packet_rtp_info_, &rtp_info, sizeof(rtp_info));
}

void InitialDelayManager::LatePackets(
    uint32_t timestamp_now, SyncStream* sync_stream) {
  assert(sync_stream);
  sync_stream->num_sync_packets = 0;

  
  
  
  
  
  
  if (timestamp_step_ <= 0 ||
      last_packet_type_ == kCngPacket ||
      last_packet_type_ == kUndefinedPacket ||
      audio_payload_type_ == kInvalidPayloadType)  
    return;

  int num_late_packets = (timestamp_now - last_receive_timestamp_) /
      timestamp_step_;

  if (num_late_packets < late_packet_threshold_)
    return;

  int sync_offset = 1;  
  if (last_packet_type_ != kSyncPacket) {
    ++sync_offset;  
    --num_late_packets;
  }
  uint32_t timestamp_update = sync_offset * timestamp_step_;

  sync_stream->num_sync_packets = num_late_packets;
  if (num_late_packets == 0)
    return;

  
  memcpy(&sync_stream->rtp_info, &last_packet_rtp_info_,
         sizeof(last_packet_rtp_info_));

  
  sync_stream->rtp_info.header.sequenceNumber += sync_offset;
  sync_stream->rtp_info.header.timestamp += timestamp_update;
  sync_stream->receive_timestamp = last_receive_timestamp_ + timestamp_update;
  sync_stream->timestamp_step = timestamp_step_;

  
  sync_stream->rtp_info.header.payloadType = audio_payload_type_;

  uint16_t sequence_number_update = num_late_packets + sync_offset - 1;
  timestamp_update = sequence_number_update * timestamp_step_;

  
  last_packet_rtp_info_.header.timestamp += timestamp_update;
  last_packet_rtp_info_.header.sequenceNumber += sequence_number_update;
  last_packet_rtp_info_.header.payloadType = audio_payload_type_;
  last_receive_timestamp_ += timestamp_update;

  last_packet_type_ = kSyncPacket;
  return;
}

void InitialDelayManager::DisableBuffering() {
  buffering_ = false;
}

void InitialDelayManager::UpdatePlayoutTimestamp(
    const RTPHeader& current_header, int sample_rate_hz) {
  playout_timestamp_ = current_header.timestamp - static_cast<uint32_t>(
      initial_delay_ms_ * sample_rate_hz / 1000);
}

}  

}  
