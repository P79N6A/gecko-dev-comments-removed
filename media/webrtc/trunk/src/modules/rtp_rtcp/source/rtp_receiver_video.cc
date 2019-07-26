









#include "rtp_receiver_video.h"

#include <cassert> 
#include <cstring>  
#include <math.h>

#include "critical_section_wrapper.h"
#include "receiver_fec.h"
#include "rtp_rtcp_impl.h"
#include "rtp_utility.h"
#include "trace.h"

namespace webrtc {
WebRtc_UWord32 BitRateBPS(WebRtc_UWord16 x )
{
    return (x & 0x3fff) * WebRtc_UWord32(pow(10.0f,(2 + (x >> 14))));
}

RTPReceiverVideo::RTPReceiverVideo(const WebRtc_Word32 id,
                                   RemoteBitrateEstimator* remote_bitrate,
                                   ModuleRtpRtcpImpl* owner)
    : _id(id),
      _criticalSectionReceiverVideo(
          CriticalSectionWrapper::CreateCriticalSection()),
      _currentFecFrameDecoded(false),
      _receiveFEC(NULL),
      remote_bitrate_(remote_bitrate),
      _packetOverHead(28) {
}

RTPReceiverVideo::~RTPReceiverVideo() {
    delete _criticalSectionReceiverVideo;
    delete _receiveFEC;
}

ModuleRTPUtility::Payload* RTPReceiverVideo::RegisterReceiveVideoPayload(
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const WebRtc_Word8 payloadType,
    const WebRtc_UWord32 maxRate) {
  RtpVideoCodecTypes videoType = kRtpNoVideo;
  if (ModuleRTPUtility::StringCompare(payloadName, "VP8", 3)) {
    videoType = kRtpVp8Video;
  } else if (ModuleRTPUtility::StringCompare(payloadName, "I420", 4)) {
    videoType = kRtpNoVideo;
  } else if (ModuleRTPUtility::StringCompare(payloadName, "ULPFEC", 6)) {
    
    if (_receiveFEC == NULL) {
      _receiveFEC = new ReceiverFEC(_id, this);
    }
    _receiveFEC->SetPayloadTypeFEC(payloadType);
    videoType = kRtpFecVideo;
  } else {
    return NULL;
  }
  ModuleRTPUtility::Payload* payload =  new ModuleRTPUtility::Payload;

  payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
  strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
  payload->typeSpecific.Video.videoCodecType = videoType;
  payload->typeSpecific.Video.maxRate = maxRate;
  payload->audio = false;
  return payload;
}




WebRtc_Word32 RTPReceiverVideo::ParseVideoCodecSpecific(
    WebRtcRTPHeader* rtpHeader,
    const WebRtc_UWord8* payloadData,
    const WebRtc_UWord16 payloadDataLength,
    const RtpVideoCodecTypes videoType,
    const bool isRED,
    const WebRtc_UWord8* incomingRtpPacket,
    const WebRtc_UWord16 incomingRtpPacketSize,
    const WebRtc_Word64 nowMS) {
  WebRtc_Word32 retVal = 0;

  _criticalSectionReceiverVideo->Enter();

  
  
  const WebRtc_UWord16 packetSize = payloadDataLength + _packetOverHead +
      rtpHeader->header.headerLength + rtpHeader->header.paddingLength;
  uint32_t compensated_timestamp = rtpHeader->header.timestamp +
      rtpHeader->extension.transmissionTimeOffset;
  remote_bitrate_->IncomingPacket(rtpHeader->header.ssrc,
                                  packetSize,
                                  nowMS,
                                  compensated_timestamp,
                                  -1);

  if (isRED) {
    if(_receiveFEC == NULL) {
      _criticalSectionReceiverVideo->Leave();
      return -1;
    }
    bool FECpacket = false;
    retVal = _receiveFEC->AddReceivedFECPacket(
        rtpHeader,
        incomingRtpPacket,
        payloadDataLength,
        FECpacket);
    if (retVal != -1) {
      retVal = _receiveFEC->ProcessReceivedFEC();
    }
    _criticalSectionReceiverVideo->Leave();

    if(retVal == 0 && FECpacket) {
      
      
      
      
      rtpHeader->frameType = kFrameEmpty;
      
      WebRtc_Word32 retVal = SetCodecType(videoType, rtpHeader);
      if(retVal != 0) {
        return retVal;
      }
      retVal = CallbackOfReceivedPayloadData(NULL, 0, rtpHeader);
    }
  } else {
    
    retVal = ParseVideoCodecSpecificSwitch(rtpHeader,
                                           payloadData,
                                           payloadDataLength,
                                           videoType);
  }
  return retVal;
}

WebRtc_Word32 RTPReceiverVideo::BuildRTPheader(
    const WebRtcRTPHeader* rtpHeader,
    WebRtc_UWord8* dataBuffer) const {
  dataBuffer[0] = static_cast<WebRtc_UWord8>(0x80);  
  dataBuffer[1] = static_cast<WebRtc_UWord8>(rtpHeader->header.payloadType);
  if (rtpHeader->header.markerBit) {
    dataBuffer[1] |= kRtpMarkerBitMask;  
  }
  ModuleRTPUtility::AssignUWord16ToBuffer(dataBuffer + 2,
                                          rtpHeader->header.sequenceNumber);
  ModuleRTPUtility::AssignUWord32ToBuffer(dataBuffer + 4,
                                          rtpHeader->header.timestamp);
  ModuleRTPUtility::AssignUWord32ToBuffer(dataBuffer + 8,
                                          rtpHeader->header.ssrc);

  WebRtc_Word32 rtpHeaderLength = 12;

  
  if (rtpHeader->header.numCSRCs > 0) {
    if (rtpHeader->header.numCSRCs > 16) {
      
      assert(false);
    }
    WebRtc_UWord8* ptr = &dataBuffer[rtpHeaderLength];
    for (WebRtc_UWord32 i = 0; i < rtpHeader->header.numCSRCs; ++i) {
      ModuleRTPUtility::AssignUWord32ToBuffer(ptr,
                                              rtpHeader->header.arrOfCSRCs[i]);
      ptr +=4;
    }
    dataBuffer[0] = (dataBuffer[0]&0xf0) | rtpHeader->header.numCSRCs;
    
    rtpHeaderLength += sizeof(WebRtc_UWord32)*rtpHeader->header.numCSRCs;
  }
  return rtpHeaderLength;
}

WebRtc_Word32 RTPReceiverVideo::ReceiveRecoveredPacketCallback(
    WebRtcRTPHeader* rtpHeader,
    const WebRtc_UWord8* payloadData,
    const WebRtc_UWord16 payloadDataLength) {
  
  _criticalSectionReceiverVideo->Enter();

  _currentFecFrameDecoded = true;

  ModuleRTPUtility::Payload* payload = NULL;
  if (PayloadTypeToPayload(rtpHeader->header.payloadType, payload) != 0) {
    _criticalSectionReceiverVideo->Leave();
    return -1;
  }
  
  
  WebRtc_UWord8 recoveredPacket[IP_PACKET_SIZE];
  WebRtc_UWord16 rtpHeaderLength = (WebRtc_UWord16)BuildRTPheader(
      rtpHeader, recoveredPacket);

  const WebRtc_UWord8 REDForFECHeaderLength = 1;

  
  recoveredPacket[1] &= 0x80;             
  recoveredPacket[1] += REDPayloadType(); 

  
  recoveredPacket[rtpHeaderLength] = rtpHeader->header.payloadType;
  

  memcpy(recoveredPacket + rtpHeaderLength + REDForFECHeaderLength, payloadData,
         payloadDataLength);

  return ParseVideoCodecSpecificSwitch(
      rtpHeader,
      payloadData,
      payloadDataLength,
      payload->typeSpecific.Video.videoCodecType);
}

WebRtc_Word32 RTPReceiverVideo::SetCodecType(const RtpVideoCodecTypes videoType,
                                             WebRtcRTPHeader* rtpHeader) const {
  switch (videoType) {
    case kRtpNoVideo:
      rtpHeader->type.Video.codec = kRTPVideoGeneric;
      break;
    case kRtpVp8Video:
      rtpHeader->type.Video.codec = kRTPVideoVP8;
      break;
    case kRtpFecVideo:
      rtpHeader->type.Video.codec = kRTPVideoFEC;
      break;
  }
  return 0;
}

WebRtc_Word32 RTPReceiverVideo::ParseVideoCodecSpecificSwitch(
    WebRtcRTPHeader* rtpHeader,
    const WebRtc_UWord8* payloadData,
    const WebRtc_UWord16 payloadDataLength,
    const RtpVideoCodecTypes videoType) {
  WebRtc_Word32 retVal = SetCodecType(videoType, rtpHeader);
  if (retVal != 0) {
    _criticalSectionReceiverVideo->Leave();
    return retVal;
  }
  WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, _id, "%s(timestamp:%u)",
               __FUNCTION__, rtpHeader->header.timestamp);

  
  
  switch (videoType) {
    case kRtpNoVideo:
      return ReceiveGenericCodec(rtpHeader, payloadData, payloadDataLength);
    case kRtpVp8Video:
      return ReceiveVp8Codec(rtpHeader, payloadData, payloadDataLength);
    case kRtpFecVideo:
      break;
  }
  _criticalSectionReceiverVideo->Leave();
  return -1;
}

WebRtc_Word32 RTPReceiverVideo::ReceiveVp8Codec(
    WebRtcRTPHeader* rtpHeader,
    const WebRtc_UWord8* payloadData,
    const WebRtc_UWord16 payloadDataLength) {
  bool success;
  ModuleRTPUtility::RTPPayload parsedPacket;
  if (payloadDataLength == 0) {
    success = true;
    parsedPacket.info.VP8.dataLength = 0;
  } else {
    ModuleRTPUtility::RTPPayloadParser rtpPayloadParser(kRtpVp8Video,
                                                        payloadData,
                                                        payloadDataLength,
                                                        _id);

    success = rtpPayloadParser.Parse(parsedPacket);
  }
  
  _criticalSectionReceiverVideo->Leave();

  if (!success) {
    return -1;
  }
  if (parsedPacket.info.VP8.dataLength == 0) {
    
    
    rtpHeader->frameType = kFrameEmpty;
    if (CallbackOfReceivedPayloadData(NULL, 0, rtpHeader) != 0) {
      return -1;
    }
    return 0;
  }
  rtpHeader->frameType = (parsedPacket.frameType == ModuleRTPUtility::kIFrame) ?
      kVideoFrameKey : kVideoFrameDelta;

  RTPVideoHeaderVP8 *toHeader = &rtpHeader->type.Video.codecHeader.VP8;
  ModuleRTPUtility::RTPPayloadVP8 *fromHeader = &parsedPacket.info.VP8;

  rtpHeader->type.Video.isFirstPacket = fromHeader->beginningOfPartition
      && (fromHeader->partitionID == 0);
  toHeader->pictureId = fromHeader->hasPictureID ? fromHeader->pictureID :
      kNoPictureId;
  toHeader->tl0PicIdx = fromHeader->hasTl0PicIdx ? fromHeader->tl0PicIdx :
      kNoTl0PicIdx;
  if (fromHeader->hasTID) {
    toHeader->temporalIdx = fromHeader->tID;
    toHeader->layerSync = fromHeader->layerSync;
  } else {
    toHeader->temporalIdx = kNoTemporalIdx;
    toHeader->layerSync = false;
  }
  toHeader->keyIdx = fromHeader->hasKeyIdx ? fromHeader->keyIdx : kNoKeyIdx;

  toHeader->frameWidth = fromHeader->frameWidth;
  toHeader->frameHeight = fromHeader->frameHeight;

  toHeader->partitionId = fromHeader->partitionID;
  toHeader->beginningOfPartition = fromHeader->beginningOfPartition;

  if(CallbackOfReceivedPayloadData(parsedPacket.info.VP8.data,
                                   parsedPacket.info.VP8.dataLength,
                                   rtpHeader) != 0) {
    return -1;
  }
  return 0;
}


WebRtc_Word32 RTPReceiverVideo::ReceiveGenericCodec(
    WebRtcRTPHeader* rtpHeader,
    const WebRtc_UWord8* payloadData,
    const WebRtc_UWord16 payloadDataLength) {
  rtpHeader->frameType = kVideoFrameKey;

  if(((SequenceNumber() + 1) == rtpHeader->header.sequenceNumber) &&
      (TimeStamp() != rtpHeader->header.timestamp)) {
    rtpHeader->type.Video.isFirstPacket = true;
  }
  _criticalSectionReceiverVideo->Leave();

  if(CallbackOfReceivedPayloadData(payloadData, payloadDataLength,
                                   rtpHeader) != 0) {
    return -1;
  }
  return 0;
}

void RTPReceiverVideo::SetPacketOverHead(WebRtc_UWord16 packetOverHead) {
  _packetOverHead = packetOverHead;
}
} 
