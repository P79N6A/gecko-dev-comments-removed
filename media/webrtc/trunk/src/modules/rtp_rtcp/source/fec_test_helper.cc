









#include "modules/rtp_rtcp/source/fec_test_helper.h"

#include "modules/rtp_rtcp/source/rtp_utility.h"

namespace webrtc {

FrameGenerator::FrameGenerator()
    : num_packets_(0),
      seq_num_(0),
      timestamp_(0) {}

void FrameGenerator::NewFrame(int num_packets) {
  num_packets_ = num_packets;
  timestamp_ += 3000;
}

uint16_t FrameGenerator::NextSeqNum() {
  return ++seq_num_;
}

RtpPacket* FrameGenerator::NextPacket(int offset, size_t length) {
  RtpPacket* rtp_packet = new RtpPacket;
  for (size_t i = 0; i < length; ++i)
    rtp_packet->data[i + kRtpHeaderSize] = offset + i;
  rtp_packet->length = length + kRtpHeaderSize;
  memset(&rtp_packet->header, 0, sizeof(WebRtcRTPHeader));
  rtp_packet->header.frameType = kVideoFrameDelta;
  rtp_packet->header.header.headerLength = kRtpHeaderSize;
  rtp_packet->header.header.markerBit = (num_packets_ == 1);
  rtp_packet->header.header.sequenceNumber = seq_num_;
  rtp_packet->header.header.timestamp = timestamp_;
  rtp_packet->header.header.payloadType = kVp8PayloadType;
  BuildRtpHeader(rtp_packet->data, &rtp_packet->header.header);
  ++seq_num_;
  --num_packets_;
  return rtp_packet;
}


RtpPacket* FrameGenerator::BuildMediaRedPacket(const RtpPacket* packet) {
  const int kHeaderLength = packet->header.header.headerLength;
  RtpPacket* red_packet = new RtpPacket;
  red_packet->header = packet->header;
  red_packet->length = packet->length + 1;  
  memset(red_packet->data, 0, red_packet->length);
  
  memcpy(red_packet->data, packet->data, kHeaderLength);
  SetRedHeader(red_packet, red_packet->data[1] & 0x7f, kHeaderLength);
  memcpy(red_packet->data + kHeaderLength + 1, packet->data + kHeaderLength,
         packet->length - kHeaderLength);
  return red_packet;
}




RtpPacket* FrameGenerator::BuildFecRedPacket(const Packet* packet) {
  
  ++num_packets_;
  RtpPacket* red_packet = NextPacket(0, packet->length + 1);
  red_packet->data[1] &= ~0x80;  
  const int kHeaderLength = red_packet->header.header.headerLength;
  SetRedHeader(red_packet, kFecPayloadType, kHeaderLength);
  memcpy(red_packet->data + kHeaderLength + 1, packet->data,
         packet->length);
  red_packet->length = kHeaderLength + 1 + packet->length;
  return red_packet;
}

void FrameGenerator::SetRedHeader(Packet* red_packet, uint8_t payload_type,
                                  int header_length) const {
  
  red_packet->data[1] &= 0x80;  
  red_packet->data[1] += kRedPayloadType;  

  
  red_packet->data[header_length] = payload_type;
}

void FrameGenerator::BuildRtpHeader(uint8_t* data, const RTPHeader* header) {
  data[0] = 0x80;  
  data[1] = header->payloadType;
  data[1] |= (header->markerBit ? kRtpMarkerBitMask : 0);
  ModuleRTPUtility::AssignUWord16ToBuffer(data+2, header->sequenceNumber);
  ModuleRTPUtility::AssignUWord32ToBuffer(data+4, header->timestamp);
  ModuleRTPUtility::AssignUWord32ToBuffer(data+8, header->ssrc);
}

}  
