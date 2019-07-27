









#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"

#include <assert.h>
#include <math.h>  
#include <string.h>  

#if defined(_WIN32)

#include <Windows.h>  

#include <WinSock.h>  

#include <MMSystem.h>  
#elif ((defined WEBRTC_LINUX) || (defined WEBRTC_BSD) || (defined WEBRTC_MAC))
#include <sys/time.h>  
#include <time.h>
#endif
#if (defined(_DEBUG) && defined(_WIN32) && (_MSC_VER >= 1400))
#include <stdio.h>
#endif

#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/system_wrappers/interface/logging.h"

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

namespace RtpUtility {

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
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_BSD) || defined(WEBRTC_MAC)
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

RtpHeaderParser::RtpHeaderParser(const uint8_t* rtpData,
                                 const size_t rtpDataLength)
    : _ptrRTPDataBegin(rtpData),
      _ptrRTPDataEnd(rtpData ? (rtpData + rtpDataLength) : NULL) {
}

RtpHeaderParser::~RtpHeaderParser() {
}

bool RtpHeaderParser::RTCP() const {
  
  
  
  
  
















  






  










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

bool RtpHeaderParser::ParseRtcp(RTPHeader* header) const {
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

bool RtpHeaderParser::Parse(RTPHeader& header,
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

  
  header.extension.hasAudioLevel = false;
  header.extension.audioLevel = 0;

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

void RtpHeaderParser::ParseOneByteExtensionHeader(
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
      LOG(LS_WARNING)
          << "RTP extension header 15 encountered. Terminate parsing.";
      return;
    }

    RTPExtensionType type;
    if (ptrExtensionMap->GetType(id, &type) != 0) {
      
      LOG(LS_WARNING) << "Failed to find extension id: "
                      << static_cast<int>(id);
    } else {
      switch (type) {
        case kRtpExtensionTransmissionTimeOffset: {
          if (len != 2) {
            LOG(LS_WARNING) << "Incorrect transmission time offset len: "
                            << len;
            return;
          }
          
          
          
          
          

          int32_t transmissionTimeOffset = ptr[0] << 16;
          transmissionTimeOffset += ptr[1] << 8;
          transmissionTimeOffset += ptr[2];
          header.extension.transmissionTimeOffset =
              transmissionTimeOffset;
          if (transmissionTimeOffset & 0x800000) {
            
            header.extension.transmissionTimeOffset |= 0xFF000000;
          }
          header.extension.hasTransmissionTimeOffset = true;
          break;
        }
        case kRtpExtensionAudioLevel: {
          if (len != 0) {
            LOG(LS_WARNING) << "Incorrect audio level len: " << len;
            return;
          }
          
          
          
          
          
          

          
          
          
          
          

          header.extension.audioLevel = ptr[0];
          header.extension.hasAudioLevel = true;
          break;
        }
        case kRtpExtensionAbsoluteSendTime: {
          if (len != 2) {
            LOG(LS_WARNING) << "Incorrect absolute send time len: " << len;
            return;
          }
          
          
          
          
          

          uint32_t absoluteSendTime = ptr[0] << 16;
          absoluteSendTime += ptr[1] << 8;
          absoluteSendTime += ptr[2];
          header.extension.absoluteSendTime = absoluteSendTime;
          header.extension.hasAbsoluteSendTime = true;
          break;
        }
        default: {
          LOG(LS_WARNING) << "Extension type not implemented: " << type;
          return;
        }
      }
    }
    ptr += (len + 1);
    uint8_t num_bytes = ParsePaddingBytes(ptrRTPDataExtensionEnd, ptr);
    ptr += num_bytes;
  }
}

uint8_t RtpHeaderParser::ParsePaddingBytes(
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
}  

}  
