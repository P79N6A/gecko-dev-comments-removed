









#include "webrtc/modules/rtp_rtcp/source/receiver_fec.h"

#include <cassert>

#include "webrtc/modules/rtp_rtcp/source/rtp_receiver_video.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/trace.h"


namespace webrtc {
ReceiverFEC::ReceiverFEC(const int32_t id, RTPReceiverVideo* owner)
    : id_(id),
      owner_(owner),
      fec_(new ForwardErrorCorrection(id)),
      payload_type_fec_(-1) {}

ReceiverFEC::~ReceiverFEC() {
  
  while (!received_packet_list_.empty()) {
    ForwardErrorCorrection::ReceivedPacket* received_packet =
        received_packet_list_.front();
    delete received_packet;
    received_packet_list_.pop_front();
  }
  assert(received_packet_list_.empty());

  if (fec_ != NULL) {
    fec_->ResetState(&recovered_packet_list_);
    delete fec_;
  }
}

void ReceiverFEC::SetPayloadTypeFEC(const int8_t payload_type) {
  payload_type_fec_ = payload_type;
}




























int32_t ReceiverFEC::AddReceivedFECPacket(const WebRtcRTPHeader* rtp_header,
                                          const uint8_t* incoming_rtp_packet,
                                          const uint16_t payload_data_length,
                                          bool& FECpacket) {
  if (payload_type_fec_ == -1) {
    return -1;
  }

  uint8_t REDHeaderLength = 1;

  
  

  ForwardErrorCorrection::ReceivedPacket* received_packet =
      new ForwardErrorCorrection::ReceivedPacket;
  received_packet->pkt = new ForwardErrorCorrection::Packet;

  
  uint8_t payload_type =
      incoming_rtp_packet[rtp_header->header.headerLength] & 0x7f;

  
  if (payload_type_fec_ == payload_type) {
    received_packet->is_fec = true;
    FECpacket = true;
  } else {
    received_packet->is_fec = false;
    FECpacket = false;
  }
  received_packet->seq_num = rtp_header->header.sequenceNumber;

  uint16_t blockLength = 0;
  if (incoming_rtp_packet[rtp_header->header.headerLength] & 0x80) {
    
    REDHeaderLength = 4;
    uint16_t timestamp_offset =
        (incoming_rtp_packet[rtp_header->header.headerLength + 1]) << 8;
    timestamp_offset +=
        incoming_rtp_packet[rtp_header->header.headerLength + 2];
    timestamp_offset = timestamp_offset >> 2;
    if (timestamp_offset != 0) {
      
      
      WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, id_,
                   "Corrupt payload found in %s", __FUNCTION__);
      delete received_packet;
      return -1;
    }

    blockLength =
        (0x03 & incoming_rtp_packet[rtp_header->header.headerLength + 2]) << 8;
    blockLength += (incoming_rtp_packet[rtp_header->header.headerLength + 3]);

    
    if (incoming_rtp_packet[rtp_header->header.headerLength + 4] & 0x80) {
      
      delete received_packet;
      assert(false);
      return -1;
    }
    if (blockLength > payload_data_length - REDHeaderLength) {
      
      delete received_packet;
      assert(false);
      return -1;
    }
  }

  ForwardErrorCorrection::ReceivedPacket* second_received_packet = NULL;
  if (blockLength > 0) {
    
    REDHeaderLength = 5;

    
    memcpy(received_packet->pkt->data, incoming_rtp_packet,
           rtp_header->header.headerLength);

    
    received_packet->pkt->data[1] &= 0x80;  
    received_packet->pkt->data[1] +=
        payload_type;                       

    
    memcpy(
        received_packet->pkt->data + rtp_header->header.headerLength,
        incoming_rtp_packet + rtp_header->header.headerLength + REDHeaderLength,
        blockLength);

    received_packet->pkt->length = blockLength;

    second_received_packet = new ForwardErrorCorrection::ReceivedPacket;
    second_received_packet->pkt = new ForwardErrorCorrection::Packet;

    second_received_packet->is_fec = true;
    second_received_packet->seq_num = rtp_header->header.sequenceNumber;

    
    memcpy(second_received_packet->pkt->data,
           incoming_rtp_packet + rtp_header->header.headerLength +
               REDHeaderLength + blockLength,
           payload_data_length - REDHeaderLength - blockLength);

    second_received_packet->pkt->length =
        payload_data_length - REDHeaderLength - blockLength;

  } else if (received_packet->is_fec) {
    
    memcpy(
        received_packet->pkt->data,
        incoming_rtp_packet + rtp_header->header.headerLength + REDHeaderLength,
        payload_data_length - REDHeaderLength);
    received_packet->pkt->length = payload_data_length - REDHeaderLength;
    received_packet->ssrc =
        ModuleRTPUtility::BufferToUWord32(&incoming_rtp_packet[8]);

  } else {
    
    memcpy(received_packet->pkt->data, incoming_rtp_packet,
           rtp_header->header.headerLength);

    
    received_packet->pkt->data[1] &= 0x80;  
    received_packet->pkt->data[1] +=
        payload_type;                       

    
    memcpy(
        received_packet->pkt->data + rtp_header->header.headerLength,
        incoming_rtp_packet + rtp_header->header.headerLength + REDHeaderLength,
        payload_data_length - REDHeaderLength);

    received_packet->pkt->length =
        rtp_header->header.headerLength + payload_data_length - REDHeaderLength;
  }

  if (received_packet->pkt->length == 0) {
    delete second_received_packet;
    delete received_packet;
    return 0;
  }

  received_packet_list_.push_back(received_packet);
  if (second_received_packet) {
    received_packet_list_.push_back(second_received_packet);
  }
  return 0;
}

int32_t ReceiverFEC::ProcessReceivedFEC() {
  if (!received_packet_list_.empty()) {
    
    if (!received_packet_list_.front()->is_fec) {
      if (ParseAndReceivePacket(received_packet_list_.front()->pkt) != 0) {
        return -1;
      }
    }
    if (fec_->DecodeFEC(&received_packet_list_, &recovered_packet_list_) != 0) {
      return -1;
    }
    assert(received_packet_list_.empty());
  }
  
  ForwardErrorCorrection::RecoveredPacketList::iterator it =
      recovered_packet_list_.begin();
  for (; it != recovered_packet_list_.end(); ++it) {
    if ((*it)->returned)  
      continue;
    if (ParseAndReceivePacket((*it)->pkt) != 0) {
      return -1;
    }
    (*it)->returned = true;
  }
  return 0;
}

int ReceiverFEC::ParseAndReceivePacket(
    const ForwardErrorCorrection::Packet* packet) {
  WebRtcRTPHeader header;
  memset(&header, 0, sizeof(header));
  ModuleRTPUtility::RTPHeaderParser parser(packet->data, packet->length);
  if (!parser.Parse(header.header)) {
    return -1;
  }
  if (owner_->ReceiveRecoveredPacketCallback(
          &header, &packet->data[header.header.headerLength],
          packet->length - header.header.headerLength) != 0) {
    return -1;
  }
  return 0;
}

}  
