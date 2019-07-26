









#include "webrtc/modules/rtp_rtcp/source/fec_receiver_impl.h"

#include <assert.h>

#include "webrtc/modules/rtp_rtcp/source/rtp_receiver_video.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/trace.h"


namespace webrtc {

FecReceiver* FecReceiver::Create(int32_t id, RtpData* callback) {
  return new FecReceiverImpl(id, callback);
}

FecReceiverImpl::FecReceiverImpl(const int32_t id, RtpData* callback)
    : id_(id),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      recovered_packet_callback_(callback),
      fec_(new ForwardErrorCorrection(id)) {}

FecReceiverImpl::~FecReceiverImpl() {
  while (!received_packet_list_.empty()) {
    delete received_packet_list_.front();
    received_packet_list_.pop_front();
  }
  if (fec_ != NULL) {
    fec_->ResetState(&recovered_packet_list_);
    delete fec_;
  }
}





























int32_t FecReceiverImpl::AddReceivedRedPacket(
    const RTPHeader& header, const uint8_t* incoming_rtp_packet,
    int packet_length, uint8_t ulpfec_payload_type) {
  CriticalSectionScoped cs(crit_sect_.get());
  uint8_t REDHeaderLength = 1;
  uint16_t payload_data_length = packet_length - header.headerLength;

  
  

  ForwardErrorCorrection::ReceivedPacket* received_packet =
      new ForwardErrorCorrection::ReceivedPacket;
  received_packet->pkt = new ForwardErrorCorrection::Packet;

  
  uint8_t payload_type =
      incoming_rtp_packet[header.headerLength] & 0x7f;

  received_packet->is_fec = payload_type == ulpfec_payload_type;
  received_packet->seq_num = header.sequenceNumber;

  uint16_t blockLength = 0;
  if (incoming_rtp_packet[header.headerLength] & 0x80) {
    
    REDHeaderLength = 4;
    uint16_t timestamp_offset =
        (incoming_rtp_packet[header.headerLength + 1]) << 8;
    timestamp_offset +=
        incoming_rtp_packet[header.headerLength + 2];
    timestamp_offset = timestamp_offset >> 2;
    if (timestamp_offset != 0) {
      
      
      WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, id_,
                   "Corrupt payload found in %s", __FUNCTION__);
      delete received_packet;
      return -1;
    }

    blockLength =
        (0x03 & incoming_rtp_packet[header.headerLength + 2]) << 8;
    blockLength += (incoming_rtp_packet[header.headerLength + 3]);

    
    if (incoming_rtp_packet[header.headerLength + 4] & 0x80) {
      
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
           header.headerLength);

    
    received_packet->pkt->data[1] &= 0x80;  
    received_packet->pkt->data[1] +=
        payload_type;                       

    
    memcpy(
        received_packet->pkt->data + header.headerLength,
        incoming_rtp_packet + header.headerLength + REDHeaderLength,
        blockLength);

    received_packet->pkt->length = blockLength;

    second_received_packet = new ForwardErrorCorrection::ReceivedPacket;
    second_received_packet->pkt = new ForwardErrorCorrection::Packet;

    second_received_packet->is_fec = true;
    second_received_packet->seq_num = header.sequenceNumber;

    
    memcpy(second_received_packet->pkt->data,
           incoming_rtp_packet + header.headerLength +
               REDHeaderLength + blockLength,
           payload_data_length - REDHeaderLength - blockLength);

    second_received_packet->pkt->length =
        payload_data_length - REDHeaderLength - blockLength;

  } else if (received_packet->is_fec) {
    
    memcpy(
        received_packet->pkt->data,
        incoming_rtp_packet + header.headerLength + REDHeaderLength,
        payload_data_length - REDHeaderLength);
    received_packet->pkt->length = payload_data_length - REDHeaderLength;
    received_packet->ssrc =
        ModuleRTPUtility::BufferToUWord32(&incoming_rtp_packet[8]);

  } else {
    
    memcpy(received_packet->pkt->data, incoming_rtp_packet,
           header.headerLength);

    
    received_packet->pkt->data[1] &= 0x80;  
    received_packet->pkt->data[1] +=
        payload_type;                       

    
    memcpy(
        received_packet->pkt->data + header.headerLength,
        incoming_rtp_packet + header.headerLength + REDHeaderLength,
        payload_data_length - REDHeaderLength);

    received_packet->pkt->length =
        header.headerLength + payload_data_length - REDHeaderLength;
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

int32_t FecReceiverImpl::ProcessReceivedFec() {
  crit_sect_->Enter();
  if (!received_packet_list_.empty()) {
    
    if (!received_packet_list_.front()->is_fec) {
      ForwardErrorCorrection::Packet* packet =
          received_packet_list_.front()->pkt;
      crit_sect_->Leave();
      if (!recovered_packet_callback_->OnRecoveredPacket(packet->data,
                                                         packet->length)) {
        return -1;
      }
      crit_sect_->Enter();
    }
    if (fec_->DecodeFEC(&received_packet_list_, &recovered_packet_list_) != 0) {
      crit_sect_->Leave();
      return -1;
    }
    assert(received_packet_list_.empty());
  }
  
  ForwardErrorCorrection::RecoveredPacketList::iterator it =
      recovered_packet_list_.begin();
  for (; it != recovered_packet_list_.end(); ++it) {
    if ((*it)->returned)  
      continue;
    ForwardErrorCorrection::Packet* packet = (*it)->pkt;
    crit_sect_->Leave();
    if (!recovered_packet_callback_->OnRecoveredPacket(packet->data,
                                                       packet->length)) {
      return -1;
    }
    crit_sect_->Enter();
    (*it)->returned = true;
  }
  crit_sect_->Leave();
  return 0;
}

}  
