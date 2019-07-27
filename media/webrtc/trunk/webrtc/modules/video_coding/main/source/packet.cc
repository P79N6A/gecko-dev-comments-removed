









#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/source/packet.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_h264.h"

#include <assert.h>

namespace webrtc {

VCMPacket::VCMPacket()
  :
    payloadType(0),
    timestamp(0),
    ntp_time_ms_(0),
    seqNum(0),
    dataPtr(NULL),
    sizeBytes(0),
    markerBit(false),
    frameType(kFrameEmpty),
    codec(kVideoCodecUnknown),
    isFirstPacket(false),
    completeNALU(kNaluUnset),
    insertStartCode(false),
    width(0),
    height(0),
    codecSpecificHeader() {
}

VCMPacket::VCMPacket(const uint8_t* ptr,
                     const uint32_t size,
                     const WebRtcRTPHeader& rtpHeader) :
    payloadType(rtpHeader.header.payloadType),
    timestamp(rtpHeader.header.timestamp),
    ntp_time_ms_(rtpHeader.ntp_time_ms),
    seqNum(rtpHeader.header.sequenceNumber),
    dataPtr(ptr),
    sizeBytes(size),
    markerBit(rtpHeader.header.markerBit),

    frameType(rtpHeader.frameType),
    codec(kVideoCodecUnknown),
    isFirstPacket(rtpHeader.type.Video.isFirstPacket),
    completeNALU(kNaluComplete),
    insertStartCode(false),
    width(rtpHeader.type.Video.width),
    height(rtpHeader.type.Video.height),
    codecSpecificHeader(rtpHeader.type.Video)
{
    CopyCodecSpecifics(rtpHeader.type.Video);
}

VCMPacket::VCMPacket(const uint8_t* ptr, uint32_t size, uint16_t seq, uint32_t ts, bool mBit) :
    payloadType(0),
    timestamp(ts),
    ntp_time_ms_(0),
    seqNum(seq),
    dataPtr(ptr),
    sizeBytes(size),
    markerBit(mBit),

    frameType(kVideoFrameDelta),
    codec(kVideoCodecUnknown),
    isFirstPacket(false),
    completeNALU(kNaluComplete),
    insertStartCode(false),
    width(0),
    height(0),
    codecSpecificHeader()
{}

void VCMPacket::Reset() {
  payloadType = 0;
  timestamp = 0;
  ntp_time_ms_ = 0;
  seqNum = 0;
  dataPtr = NULL;
  sizeBytes = 0;
  markerBit = false;
  frameType = kFrameEmpty;
  codec = kVideoCodecUnknown;
  isFirstPacket = false;
  completeNALU = kNaluUnset;
  insertStartCode = false;
  width = 0;
  height = 0;
  memset(&codecSpecificHeader, 0, sizeof(RTPVideoHeader));
}

void VCMPacket::CopyCodecSpecifics(const RTPVideoHeader& videoHeader) {
  switch (videoHeader.codec) {
    case kRtpVideoVp8:
    case kRtpVideoVp9:
      
      
      
      if (isFirstPacket && markerBit)
        completeNALU = kNaluComplete;
      else if (isFirstPacket)
        completeNALU = kNaluStart;
      else if (markerBit)
        completeNALU = kNaluEnd;
      else
        completeNALU = kNaluIncomplete;

      codec = videoHeader.codec == kRtpVideoVp8 ? kVideoCodecVP8 : kVideoCodecVP9;
      return;
    case kRtpVideoH264:
      isFirstPacket = videoHeader.isFirstPacket;
      if (isFirstPacket) {
        insertStartCode = true;
      }
      if (videoHeader.codecHeader.H264.single_nalu) {
        completeNALU = kNaluComplete;
      } else if (isFirstPacket) {
        completeNALU = kNaluStart;
      } else if (markerBit) {
        completeNALU = kNaluEnd;
      } else {
        completeNALU = kNaluIncomplete;
      }
      codec = kVideoCodecH264;
      return;
    case kRtpVideoGeneric:
    case kRtpVideoNone:
      if (isFirstPacket && markerBit)
        completeNALU = kNaluComplete;
      else if (isFirstPacket)
        completeNALU = kNaluStart;
      else if (markerBit)
        completeNALU = kNaluEnd;
      else
        completeNALU = kNaluIncomplete;
      codec = kVideoCodecUnknown;
      return;
  }
}

}  
