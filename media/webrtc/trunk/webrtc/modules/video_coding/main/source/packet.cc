









#include "packet.h"
#include "module_common_types.h"

#include <assert.h>

namespace webrtc {

VCMPacket::VCMPacket()
  :
    payloadType(0),
    timestamp(0),
    seqNum(0),
    dataPtr(NULL),
    sizeBytes(0),
    markerBit(false),
    frameType(kFrameEmpty),
    codec(kVideoCodecUnknown),
    isFirstPacket(false),
    completeNALU(kNaluUnset),
    insertStartCode(false),
    codecSpecificHeader() {
}

VCMPacket::VCMPacket(const WebRtc_UWord8* ptr,
                               const WebRtc_UWord32 size,
                               const WebRtcRTPHeader& rtpHeader) :
    payloadType(rtpHeader.header.payloadType),
    timestamp(rtpHeader.header.timestamp),
    seqNum(rtpHeader.header.sequenceNumber),
    dataPtr(ptr),
    sizeBytes(size),
    markerBit(rtpHeader.header.markerBit),

    frameType(rtpHeader.frameType),
    codec(kVideoCodecUnknown),
    isFirstPacket(rtpHeader.type.Video.isFirstPacket),
    completeNALU(kNaluComplete),
    insertStartCode(false),
    codecSpecificHeader(rtpHeader.type.Video)
{
    CopyCodecSpecifics(rtpHeader.type.Video);
}

VCMPacket::VCMPacket(const WebRtc_UWord8* ptr, WebRtc_UWord32 size, WebRtc_UWord16 seq, WebRtc_UWord32 ts, bool mBit) :
    payloadType(0),
    timestamp(ts),
    seqNum(seq),
    dataPtr(ptr),
    sizeBytes(size),
    markerBit(mBit),

    frameType(kVideoFrameDelta),
    codec(kVideoCodecUnknown),
    isFirstPacket(false),
    completeNALU(kNaluComplete),
    insertStartCode(false),
    codecSpecificHeader()
{}

void VCMPacket::Reset() {
  payloadType = 0;
  timestamp = 0;
  seqNum = 0;
  dataPtr = NULL;
  sizeBytes = 0;
  markerBit = false;
  frameType = kFrameEmpty;
  codec = kVideoCodecUnknown;
  isFirstPacket = false;
  completeNALU = kNaluUnset;
  insertStartCode = false;
  memset(&codecSpecificHeader, 0, sizeof(RTPVideoHeader));
}

void VCMPacket::CopyCodecSpecifics(const RTPVideoHeader& videoHeader)
{
    switch(videoHeader.codec)
    {
        case kRTPVideoVP8:
            {
                
                
                
                if (isFirstPacket && markerBit)
                    completeNALU = kNaluComplete;
                else if (isFirstPacket)
                    completeNALU = kNaluStart;
                else if (markerBit)
                    completeNALU = kNaluEnd;
                else
                    completeNALU = kNaluIncomplete;

                codec = kVideoCodecVP8;
                break;
            }
        case kRTPVideoI420:
            {
                codec = kVideoCodecI420;
                break;
            }
        default:
            {
                codec = kVideoCodecUnknown;
                break;
            }
    }
}

}
