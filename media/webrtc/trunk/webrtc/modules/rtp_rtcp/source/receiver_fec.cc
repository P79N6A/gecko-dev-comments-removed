









#include "modules/rtp_rtcp/source/receiver_fec.h"

#include <cassert>

#include "modules/rtp_rtcp/source/rtp_receiver_video.h"
#include "modules/rtp_rtcp/source/rtp_utility.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "system_wrappers/interface/trace.h"


namespace webrtc {
ReceiverFEC::ReceiverFEC(const WebRtc_Word32 id, RTPReceiverVideo* owner)
    : _id(id),
      _owner(owner),
      _fec(new ForwardErrorCorrection(id)),
      _payloadTypeFEC(-1) {
}

ReceiverFEC::~ReceiverFEC() {
  
  while (!_receivedPacketList.empty()){
    ForwardErrorCorrection::ReceivedPacket* receivedPacket =
        _receivedPacketList.front();
    delete receivedPacket;
    _receivedPacketList.pop_front();
  }
  assert(_receivedPacketList.empty());

  if (_fec != NULL) {
    _fec->ResetState(&_recoveredPacketList);
    delete _fec;
  }
}

void ReceiverFEC::SetPayloadTypeFEC(const WebRtc_Word8 payloadType) {
  _payloadTypeFEC = payloadType;
}































WebRtc_Word32 ReceiverFEC::AddReceivedFECPacket(
    const WebRtcRTPHeader* rtpHeader,
    const WebRtc_UWord8* incomingRtpPacket,
    const WebRtc_UWord16 payloadDataLength,
    bool& FECpacket) {
  if (_payloadTypeFEC == -1) {
    return -1;
  }

  WebRtc_UWord8 REDHeaderLength = 1;

  
  

  ForwardErrorCorrection::ReceivedPacket* receivedPacket =
      new ForwardErrorCorrection::ReceivedPacket;
  receivedPacket->pkt = new ForwardErrorCorrection::Packet;

  
  WebRtc_UWord8 payloadType =
      incomingRtpPacket[rtpHeader->header.headerLength] & 0x7f;

  
  if (_payloadTypeFEC == payloadType) {
    receivedPacket->isFec = true;
    FECpacket = true;
  } else {
    receivedPacket->isFec = false;
    FECpacket = false;
  }
  receivedPacket->seqNum = rtpHeader->header.sequenceNumber;

  WebRtc_UWord16 blockLength = 0;
  if(incomingRtpPacket[rtpHeader->header.headerLength] & 0x80) {
    
    REDHeaderLength = 4;
    WebRtc_UWord16 timestampOffset =
        (incomingRtpPacket[rtpHeader->header.headerLength + 1]) << 8;
    timestampOffset += incomingRtpPacket[rtpHeader->header.headerLength+2];
    timestampOffset = timestampOffset >> 2;
    if(timestampOffset != 0) {
      
      
      WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id,
                   "Corrupt payload found in %s", __FUNCTION__);
      delete receivedPacket;
      return -1;
    }

    blockLength =
        (0x03 & incomingRtpPacket[rtpHeader->header.headerLength + 2]) << 8;
    blockLength += (incomingRtpPacket[rtpHeader->header.headerLength + 3]);

    
    if(incomingRtpPacket[rtpHeader->header.headerLength+4] & 0x80) {
      
      delete receivedPacket;
      assert(false);
      return -1;
    }
    if(blockLength > payloadDataLength - REDHeaderLength) {
      
      delete receivedPacket;
      assert(false);
      return -1;
    }
  }

  ForwardErrorCorrection::ReceivedPacket* secondReceivedPacket = NULL;
  if (blockLength > 0) {
    
    REDHeaderLength = 5;

    
    memcpy(receivedPacket->pkt->data,
           incomingRtpPacket,
           rtpHeader->header.headerLength);

    
    receivedPacket->pkt->data[1] &= 0x80;         
    receivedPacket->pkt->data[1] += payloadType;  

    
    memcpy(receivedPacket->pkt->data + rtpHeader->header.headerLength,
           incomingRtpPacket + rtpHeader->header.headerLength + REDHeaderLength,
           blockLength);

    receivedPacket->pkt->length = blockLength;

    secondReceivedPacket = new ForwardErrorCorrection::ReceivedPacket;
    secondReceivedPacket->pkt = new ForwardErrorCorrection::Packet;

    secondReceivedPacket->isFec = true;
    secondReceivedPacket->seqNum = rtpHeader->header.sequenceNumber;

    
    memcpy(secondReceivedPacket->pkt->data,
           incomingRtpPacket + rtpHeader->header.headerLength +
               REDHeaderLength + blockLength,
           payloadDataLength - REDHeaderLength - blockLength);

    secondReceivedPacket->pkt->length = payloadDataLength - REDHeaderLength -
        blockLength;

  } else if(receivedPacket->isFec) {
    
    memcpy(receivedPacket->pkt->data,
           incomingRtpPacket + rtpHeader->header.headerLength + REDHeaderLength,
           payloadDataLength - REDHeaderLength);
    receivedPacket->pkt->length = payloadDataLength - REDHeaderLength;
    receivedPacket->ssrc =
        ModuleRTPUtility::BufferToUWord32(&incomingRtpPacket[8]);

  } else {
    
    memcpy(receivedPacket->pkt->data,
           incomingRtpPacket,
           rtpHeader->header.headerLength);

    
    receivedPacket->pkt->data[1] &= 0x80;         
    receivedPacket->pkt->data[1] += payloadType;  

    
    memcpy(receivedPacket->pkt->data + rtpHeader->header.headerLength,
           incomingRtpPacket + rtpHeader->header.headerLength + REDHeaderLength,
           payloadDataLength - REDHeaderLength);

    receivedPacket->pkt->length = rtpHeader->header.headerLength +
        payloadDataLength - REDHeaderLength;
  }

  if(receivedPacket->pkt->length == 0) {
    delete secondReceivedPacket;
    delete receivedPacket;
    return 0;
  }

  _receivedPacketList.push_back(receivedPacket);
  if (secondReceivedPacket) {
    _receivedPacketList.push_back(secondReceivedPacket);
  }
  return 0;
}

WebRtc_Word32 ReceiverFEC::ProcessReceivedFEC() {
  if (!_receivedPacketList.empty()) {
    
    if (!_receivedPacketList.front()->isFec) {
      if (ParseAndReceivePacket(_receivedPacketList.front()->pkt) != 0) {
        return -1;
      }
    }
    if (_fec->DecodeFEC(&_receivedPacketList, &_recoveredPacketList) != 0) {
      return -1;
    }
    assert(_receivedPacketList.empty());
  }
  
  ForwardErrorCorrection::RecoveredPacketList::iterator it =
      _recoveredPacketList.begin();
  for (; it != _recoveredPacketList.end(); ++it) {
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
  ModuleRTPUtility::RTPHeaderParser parser(packet->data,
                                           packet->length);
  if (!parser.Parse(header)) {
    return -1;
  }
  if (_owner->ReceiveRecoveredPacketCallback(
      &header,
      &packet->data[header.header.headerLength],
      packet->length - header.header.headerLength) != 0) {
    return -1;
  }
  return 0;
}

} 
