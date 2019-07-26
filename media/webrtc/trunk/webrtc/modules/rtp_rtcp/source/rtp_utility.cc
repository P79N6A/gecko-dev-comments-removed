









#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"

#include <assert.h>
#include <math.h>  
#include <string.h>  

#if defined(_WIN32)

#include <Windows.h>  

#include <WinSock.h>  

#include <MMSystem.h>  
#elif ((defined WEBRTC_LINUX) || (defined WEBRTC_MAC))
#include <sys/time.h>  
#include <time.h>
#endif
#if (defined(_DEBUG) && defined(_WIN32) && (_MSC_VER >= 1400))
#include <stdio.h>
#endif

#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/system_wrappers/interface/trace.h"

#if (defined(_DEBUG) && defined(_WIN32) && (_MSC_VER >= 1400))
#define DEBUG_PRINT(...)           \
  {                                \
    char msg[256];                 \
    sprintf(msg, __VA_ARGS__);     \
    OutputDebugString(msg);        \
  }
#else

#define DEBUG_PRINT(exp)        ((void)0)
#endif  

namespace webrtc {

RtpData* NullObjectRtpData() {
  static NullRtpData null_rtp_data;
  return &null_rtp_data;
}

RtpFeedback* NullObjectRtpFeedback() {
  static NullRtpFeedback null_rtp_feedback;
  return &null_rtp_feedback;
}

RtpAudioFeedback* NullObjectRtpAudioFeedback() {
  static NullRtpAudioFeedback null_rtp_audio_feedback;
  return &null_rtp_audio_feedback;
}

ReceiveStatistics* NullObjectReceiveStatistics() {
  static NullReceiveStatistics null_receive_statistics;
  return &null_receive_statistics;
}

namespace ModuleRTPUtility {

enum {
  kRtcpExpectedVersion = 2,
  kRtcpMinHeaderLength = 4,
  kRtcpMinParseLength = 8,

  kRtpExpectedVersion = 2,
  kRtpMinParseLength = 12
};





uint32_t GetCurrentRTP(Clock* clock, uint32_t freq) {
  const bool use_global_clock = (clock == NULL);
  Clock* local_clock = clock;
  if (use_global_clock) {
    local_clock = Clock::GetRealTimeClock();
  }
  uint32_t secs = 0, frac = 0;
  local_clock->CurrentNtp(secs, frac);
  if (use_global_clock) {
    delete local_clock;
  }
  return ConvertNTPTimeToRTP(secs, frac, freq);
}

uint32_t ConvertNTPTimeToRTP(uint32_t NTPsec, uint32_t NTPfrac, uint32_t freq) {
  float ftemp = (float)NTPfrac / (float)NTP_FRAC;
  uint32_t tmp = (uint32_t)(ftemp * freq);
  return NTPsec * freq + tmp;
}

uint32_t ConvertNTPTimeToMS(uint32_t NTPsec, uint32_t NTPfrac) {
  int freq = 1000;
  float ftemp = (float)NTPfrac / (float)NTP_FRAC;
  uint32_t tmp = (uint32_t)(ftemp * freq);
  uint32_t MStime = NTPsec * freq + tmp;
  return MStime;
}





#if defined(_WIN32)
bool StringCompare(const char* str1, const char* str2,
                   const uint32_t length) {
  return (_strnicmp(str1, str2, length) == 0) ? true : false;
}
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
bool StringCompare(const char* str1, const char* str2,
                   const uint32_t length) {
  return (strncasecmp(str1, str2, length) == 0) ? true : false;
}
#endif





void AssignUWord32ToBuffer(uint8_t* dataBuffer, uint32_t value) {
#if defined(WEBRTC_ARCH_LITTLE_ENDIAN)
  dataBuffer[0] = static_cast<uint8_t>(value >> 24);
  dataBuffer[1] = static_cast<uint8_t>(value >> 16);
  dataBuffer[2] = static_cast<uint8_t>(value >> 8);
  dataBuffer[3] = static_cast<uint8_t>(value);
#else
  uint32_t* ptr = reinterpret_cast<uint32_t*>(dataBuffer);
  ptr[0] = value;
#endif
}

void AssignUWord24ToBuffer(uint8_t* dataBuffer, uint32_t value) {
#if defined(WEBRTC_ARCH_LITTLE_ENDIAN)
  dataBuffer[0] = static_cast<uint8_t>(value >> 16);
  dataBuffer[1] = static_cast<uint8_t>(value >> 8);
  dataBuffer[2] = static_cast<uint8_t>(value);
#else
  dataBuffer[0] = static_cast<uint8_t>(value);
  dataBuffer[1] = static_cast<uint8_t>(value >> 8);
  dataBuffer[2] = static_cast<uint8_t>(value >> 16);
#endif
}

void AssignUWord16ToBuffer(uint8_t* dataBuffer, uint16_t value) {
#if defined(WEBRTC_ARCH_LITTLE_ENDIAN)
  dataBuffer[0] = static_cast<uint8_t>(value >> 8);
  dataBuffer[1] = static_cast<uint8_t>(value);
#else
  uint16_t* ptr = reinterpret_cast<uint16_t*>(dataBuffer);
  ptr[0] = value;
#endif
}

uint16_t BufferToUWord16(const uint8_t* dataBuffer) {
#if defined(WEBRTC_ARCH_LITTLE_ENDIAN)
  return (dataBuffer[0] << 8) + dataBuffer[1];
#else
  return *reinterpret_cast<const uint16_t*>(dataBuffer);
#endif
}

uint32_t BufferToUWord24(const uint8_t* dataBuffer) {
  return (dataBuffer[0] << 16) + (dataBuffer[1] << 8) + dataBuffer[2];
}

uint32_t BufferToUWord32(const uint8_t* dataBuffer) {
#if defined(WEBRTC_ARCH_LITTLE_ENDIAN)
  return (dataBuffer[0] << 24) + (dataBuffer[1] << 16) + (dataBuffer[2] << 8) +
      dataBuffer[3];
#else
  return *reinterpret_cast<const uint32_t*>(dataBuffer);
#endif
}

uint32_t pow2(uint8_t exp) {
  return 1 << exp;
}

void RTPPayload::SetType(RtpVideoCodecTypes videoType) {
  type = videoType;

  switch (type) {
    case kRtpVideoGeneric:
      break;
    case kRtpVideoVp8: {
      info.VP8.nonReferenceFrame = false;
      info.VP8.beginningOfPartition = false;
      info.VP8.partitionID = 0;
      info.VP8.hasPictureID = false;
      info.VP8.hasTl0PicIdx = false;
      info.VP8.hasTID = false;
      info.VP8.hasKeyIdx = false;
      info.VP8.pictureID = -1;
      info.VP8.tl0PicIdx = -1;
      info.VP8.tID = -1;
      info.VP8.layerSync = false;
      info.VP8.frameWidth = 0;
      info.VP8.frameHeight = 0;
      break;
    }
    default:
      break;
  }
}

RTPHeaderParser::RTPHeaderParser(const uint8_t* rtpData,
                                 const uint32_t rtpDataLength)
  : _ptrRTPDataBegin(rtpData),
    _ptrRTPDataEnd(rtpData ? (rtpData + rtpDataLength) : NULL) {
}

RTPHeaderParser::~RTPHeaderParser() {
}

bool RTPHeaderParser::RTCP() const {
  
  
  
  
  
















  






  










  const ptrdiff_t length = _ptrRTPDataEnd - _ptrRTPDataBegin;
  if (length < kRtcpMinHeaderLength) {
    return false;
  }

  const uint8_t V  = _ptrRTPDataBegin[0] >> 6;
  if (V != kRtcpExpectedVersion) {
    return false;
  }

  const uint8_t  payloadType = _ptrRTPDataBegin[1];
  bool RTCP = false;
  switch (payloadType) {
    case 192:
      RTCP = true;
      break;
    case 193:
      
      
      break;
    case 195:
    case 200:
    case 201:
    case 202:
    case 203:
    case 204:
    case 205:
    case 206:
    case 207:
      RTCP = true;
      break;
  }
  return RTCP;
}

bool RTPHeaderParser::ParseRtcp(RTPHeader* header) const {
  assert(header != NULL);

  const ptrdiff_t length = _ptrRTPDataEnd - _ptrRTPDataBegin;
  if (length < kRtcpMinParseLength) {
    return false;
  }

  const uint8_t V = _ptrRTPDataBegin[0] >> 6;
  if (V != kRtcpExpectedVersion) {
    return false;
  }

  const uint8_t PT = _ptrRTPDataBegin[1];
  const uint16_t len = (_ptrRTPDataBegin[2] << 8) + _ptrRTPDataBegin[3];
  const uint8_t* ptr = &_ptrRTPDataBegin[4];

  uint32_t SSRC = *ptr++ << 24;
  SSRC += *ptr++ << 16;
  SSRC += *ptr++ << 8;
  SSRC += *ptr++;

  header->payloadType  = PT;
  header->ssrc         = SSRC;
  header->headerLength = 4 + (len << 2);

  return true;
}

bool RTPHeaderParser::Parse(RTPHeader& header,
                            RtpHeaderExtensionMap* ptrExtensionMap) const {
  const ptrdiff_t length = _ptrRTPDataEnd - _ptrRTPDataBegin;
  if (length < kRtpMinParseLength) {
    return false;
  }

  
  const uint8_t V  = _ptrRTPDataBegin[0] >> 6;
  
  const bool          P  = ((_ptrRTPDataBegin[0] & 0x20) == 0) ? false : true;
  
  const bool          X  = ((_ptrRTPDataBegin[0] & 0x10) == 0) ? false : true;
  const uint8_t CC = _ptrRTPDataBegin[0] & 0x0f;
  const bool          M  = ((_ptrRTPDataBegin[1] & 0x80) == 0) ? false : true;

  const uint8_t PT = _ptrRTPDataBegin[1] & 0x7f;

  const uint16_t sequenceNumber = (_ptrRTPDataBegin[2] << 8) +
      _ptrRTPDataBegin[3];

  const uint8_t* ptr = &_ptrRTPDataBegin[4];

  uint32_t RTPTimestamp = *ptr++ << 24;
  RTPTimestamp += *ptr++ << 16;
  RTPTimestamp += *ptr++ << 8;
  RTPTimestamp += *ptr++;

  uint32_t SSRC = *ptr++ << 24;
  SSRC += *ptr++ << 16;
  SSRC += *ptr++ << 8;
  SSRC += *ptr++;

  if (V != kRtpExpectedVersion) {
    return false;
  }

  const uint8_t CSRCocts = CC * 4;

  if ((ptr + CSRCocts) > _ptrRTPDataEnd) {
    return false;
  }

  header.markerBit      = M;
  header.payloadType    = PT;
  header.sequenceNumber = sequenceNumber;
  header.timestamp      = RTPTimestamp;
  header.ssrc           = SSRC;
  header.numCSRCs       = CC;
  header.paddingLength  = P ? *(_ptrRTPDataEnd - 1) : 0;

  for (unsigned int i = 0; i < CC; ++i) {
    uint32_t CSRC = *ptr++ << 24;
    CSRC += *ptr++ << 16;
    CSRC += *ptr++ << 8;
    CSRC += *ptr++;
    header.arrOfCSRCs[i] = CSRC;
  }

  header.headerLength   = 12 + CSRCocts;

  
  
  header.extension.hasTransmissionTimeOffset = false;
  header.extension.transmissionTimeOffset = 0;

  
  header.extension.hasAbsoluteSendTime = false;
  header.extension.absoluteSendTime = 0;

  if (X) {
    








    const ptrdiff_t remain = _ptrRTPDataEnd - ptr;
    if (remain < 4) {
      return false;
    }

    header.headerLength += 4;

    uint16_t definedByProfile = *ptr++ << 8;
    definedByProfile += *ptr++;

    uint16_t XLen = *ptr++ << 8;
    XLen += *ptr++; 
    XLen *= 4; 

    if (remain < (4 + XLen)) {
      return false;
    }
    if (definedByProfile == kRtpOneByteHeaderExtensionId) {
      const uint8_t* ptrRTPDataExtensionEnd = ptr + XLen;
      ParseOneByteExtensionHeader(header,
                                  ptrExtensionMap,
                                  ptrRTPDataExtensionEnd,
                                  ptr);
    }
    header.headerLength += XLen;
  }
  return true;
}

void RTPHeaderParser::ParseOneByteExtensionHeader(
    RTPHeader& header,
    const RtpHeaderExtensionMap* ptrExtensionMap,
    const uint8_t* ptrRTPDataExtensionEnd,
    const uint8_t* ptr) const {
  if (!ptrExtensionMap) {
    return;
  }

  while (ptrRTPDataExtensionEnd - ptr > 0) {
    
    
    
    
    

    const uint8_t id = (*ptr & 0xf0) >> 4;
    const uint8_t len = (*ptr & 0x0f);
    ptr++;

    if (id == 15) {
      WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, -1,
                   "Ext id: 15 encountered, parsing terminated.");
      return;
    }

    RTPExtensionType type;
    if (ptrExtensionMap->GetType(id, &type) != 0) {
      WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
                   "Failed to find extension id: %d", id);
      return;
    }

    switch (type) {
      case kRtpExtensionTransmissionTimeOffset: {
        if (len != 2) {
          WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, -1,
                       "Incorrect transmission time offset len: %d", len);
          return;
        }
        
        
        
        
        

        int32_t transmissionTimeOffset = *ptr++ << 16;
        transmissionTimeOffset += *ptr++ << 8;
        transmissionTimeOffset += *ptr++;
        header.extension.transmissionTimeOffset =
            transmissionTimeOffset;
        if (transmissionTimeOffset & 0x800000) {
          
          header.extension.transmissionTimeOffset |= 0xFF000000;
        }
        header.extension.hasTransmissionTimeOffset = true;
        break;
      }
      case kRtpExtensionAudioLevel: {
        
        
        
        
        
        
        

        
        
        
        
        
        break;
      }
      case kRtpExtensionAbsoluteSendTime: {
        if (len != 2) {
          WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, -1,
                       "Incorrect absolute send time len: %d", len);
          return;
        }
        
        
        
        
        

        uint32_t absoluteSendTime = *ptr++ << 16;
        absoluteSendTime += *ptr++ << 8;
        absoluteSendTime += *ptr++;
        header.extension.absoluteSendTime = absoluteSendTime;
        header.extension.hasAbsoluteSendTime = true;
        break;
      }
      default: {
        WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
                     "Extension type not implemented.");
        return;
      }
    }
    uint8_t num_bytes = ParsePaddingBytes(ptrRTPDataExtensionEnd, ptr);
    ptr += num_bytes;
  }
}

uint8_t RTPHeaderParser::ParsePaddingBytes(
  const uint8_t* ptrRTPDataExtensionEnd,
  const uint8_t* ptr) const {

  uint8_t num_zero_bytes = 0;
  while (ptrRTPDataExtensionEnd - ptr > 0) {
    if (*ptr != 0) {
      return num_zero_bytes;
    }
    ptr++;
    num_zero_bytes++;
  }
  return num_zero_bytes;
}


RTPPayloadParser::RTPPayloadParser(const RtpVideoCodecTypes videoType,
                                   const uint8_t* payloadData,
                                   uint16_t payloadDataLength,
                                   int32_t id)
  :
  _id(id),
  _dataPtr(payloadData),
  _dataLength(payloadDataLength),
  _videoType(videoType) {
}

RTPPayloadParser::~RTPPayloadParser() {
}

bool RTPPayloadParser::Parse(RTPPayload& parsedPacket) const {
  parsedPacket.SetType(_videoType);

  switch (_videoType) {
    case kRtpVideoGeneric:
      return ParseGeneric(parsedPacket);
    case kRtpVideoVp8:
      return ParseVP8(parsedPacket);
    default:
      return false;
  }
}

bool RTPPayloadParser::ParseGeneric(RTPPayload& ) const {
  return false;
}


























bool RTPPayloadParser::ParseVP8(RTPPayload& parsedPacket) const {
  RTPPayloadVP8* vp8 = &parsedPacket.info.VP8;
  const uint8_t* dataPtr = _dataPtr;
  int dataLength = _dataLength;

  
  bool extension = (*dataPtr & 0x80) ? true : false;            
  vp8->nonReferenceFrame = (*dataPtr & 0x20) ? true : false;    
  vp8->beginningOfPartition = (*dataPtr & 0x10) ? true : false; 
  vp8->partitionID = (*dataPtr & 0x0F);          

  if (vp8->partitionID > 8) {
    
    return false;
  }

  
  dataPtr++;
  dataLength--;

  if (extension) {
    const int parsedBytes = ParseVP8Extension(vp8, dataPtr, dataLength);
    if (parsedBytes < 0) return false;
    dataPtr += parsedBytes;
    dataLength -= parsedBytes;
  }

  if (dataLength <= 0) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "Error parsing VP8 payload descriptor; payload too short");
    return false;
  }

  
  if (dataLength > 0 && vp8->beginningOfPartition && vp8->partitionID == 0) {
    parsedPacket.frameType = (*dataPtr & 0x01) ? kPFrame : kIFrame;
  } else {
    parsedPacket.frameType = kPFrame;
  }
  if (0 != ParseVP8FrameSize(parsedPacket, dataPtr, dataLength)) {
    return false;
  }
  parsedPacket.info.VP8.data       = dataPtr;
  parsedPacket.info.VP8.dataLength = dataLength;
  return true;
}

int RTPPayloadParser::ParseVP8FrameSize(RTPPayload& parsedPacket,
                                        const uint8_t* dataPtr,
                                        int dataLength) const {
  if (parsedPacket.frameType != kIFrame) {
    
    return 0;
  }
  if (dataLength < 10) {
    
    
    return -1;
  }
  RTPPayloadVP8* vp8 = &parsedPacket.info.VP8;
  vp8->frameWidth = ((dataPtr[7] << 8) + dataPtr[6]) & 0x3FFF;
  vp8->frameHeight = ((dataPtr[9] << 8) + dataPtr[8]) & 0x3FFF;
  return 0;
}

int RTPPayloadParser::ParseVP8Extension(RTPPayloadVP8* vp8,
                                        const uint8_t* dataPtr,
                                        int dataLength) const {
  int parsedBytes = 0;
  if (dataLength <= 0) return -1;
  
  vp8->hasPictureID = (*dataPtr & 0x80) ? true : false; 
  vp8->hasTl0PicIdx = (*dataPtr & 0x40) ? true : false; 
  vp8->hasTID = (*dataPtr & 0x20) ? true : false;       
  vp8->hasKeyIdx = (*dataPtr & 0x10) ? true : false;    

  
  dataPtr++;
  parsedBytes++;
  dataLength--;

  if (vp8->hasPictureID) {
    if (ParseVP8PictureID(vp8, &dataPtr, &dataLength, &parsedBytes) != 0) {
      return -1;
    }
  }

  if (vp8->hasTl0PicIdx) {
    if (ParseVP8Tl0PicIdx(vp8, &dataPtr, &dataLength, &parsedBytes) != 0) {
      return -1;
    }
  }

  if (vp8->hasTID || vp8->hasKeyIdx) {
    if (ParseVP8TIDAndKeyIdx(vp8, &dataPtr, &dataLength, &parsedBytes) != 0) {
      return -1;
    }
  }
  return parsedBytes;
}

int RTPPayloadParser::ParseVP8PictureID(RTPPayloadVP8* vp8,
                                        const uint8_t** dataPtr,
                                        int* dataLength,
                                        int* parsedBytes) const {
  if (*dataLength <= 0) return -1;
  vp8->pictureID = (**dataPtr & 0x7F);
  if (**dataPtr & 0x80) {
    (*dataPtr)++;
    (*parsedBytes)++;
    if (--(*dataLength) <= 0) return -1;
    
    vp8->pictureID = (vp8->pictureID << 8) +** dataPtr;
  }
  (*dataPtr)++;
  (*parsedBytes)++;
  (*dataLength)--;
  return 0;
}

int RTPPayloadParser::ParseVP8Tl0PicIdx(RTPPayloadVP8* vp8,
                                        const uint8_t** dataPtr,
                                        int* dataLength,
                                        int* parsedBytes) const {
  if (*dataLength <= 0) return -1;
  vp8->tl0PicIdx = **dataPtr;
  (*dataPtr)++;
  (*parsedBytes)++;
  (*dataLength)--;
  return 0;
}

int RTPPayloadParser::ParseVP8TIDAndKeyIdx(RTPPayloadVP8* vp8,
                                           const uint8_t** dataPtr,
                                           int* dataLength,
                                           int* parsedBytes) const {
  if (*dataLength <= 0) return -1;
  if (vp8->hasTID) {
    vp8->tID = ((**dataPtr >> 6) & 0x03);
    vp8->layerSync = (**dataPtr & 0x20) ? true : false;  
  }
  if (vp8->hasKeyIdx) {
    vp8->keyIdx = (**dataPtr & 0x1F);
  }
  (*dataPtr)++;
  (*parsedBytes)++;
  (*dataLength)--;
  return 0;
}

}  

}  
