









#include "rtp_utility.h"

#include <cassert>
#include <cmath>  
#include <cstring>  

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

#include "system_wrappers/interface/tick_util.h"
#include "system_wrappers/interface/trace.h"

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

namespace ModuleRTPUtility {





#if defined(_WIN32)

struct reference_point {
  FILETIME      file_time;
  LARGE_INTEGER counterMS;
};

struct WindowsHelpTimer {
  volatile LONG _timeInMs;
  volatile LONG _numWrapTimeInMs;
  reference_point _ref_point;

  volatile LONG _sync_flag;
};

void Synchronize(WindowsHelpTimer* help_timer) {
  const LONG start_value = 0;
  const LONG new_value = 1;
  const LONG synchronized_value = 2;

  LONG compare_flag = new_value;
  while (help_timer->_sync_flag == start_value) {
    const LONG new_value = 1;
    compare_flag = InterlockedCompareExchange(
        &help_timer->_sync_flag, new_value, start_value);
  }
  if (compare_flag != start_value) {
    
    
    while (compare_flag != synchronized_value) {
      ::Sleep(0);
    }
    return;
  }
  
  

  
  timeBeginPeriod(1);
  FILETIME    ft0 = { 0, 0 },
              ft1 = { 0, 0 };
  
  
  
  
  ::GetSystemTimeAsFileTime(&ft0);
  do {
    ::GetSystemTimeAsFileTime(&ft1);

    help_timer->_ref_point.counterMS.QuadPart = ::timeGetTime();
    ::Sleep(0);
  } while ((ft0.dwHighDateTime == ft1.dwHighDateTime) &&
          (ft0.dwLowDateTime == ft1.dwLowDateTime));
    help_timer->_ref_point.file_time = ft1;
}

void get_time(WindowsHelpTimer* help_timer, FILETIME& current_time) {
  
  DWORD t = timeGetTime();
  
  
  volatile LONG* timeInMsPtr = &help_timer->_timeInMs;
  
  DWORD old = InterlockedExchange(timeInMsPtr, t);
  if(old > t) {
    
    help_timer->_numWrapTimeInMs++;
  }
  LARGE_INTEGER elapsedMS;
  elapsedMS.HighPart = help_timer->_numWrapTimeInMs;
  elapsedMS.LowPart = t;

  elapsedMS.QuadPart = elapsedMS.QuadPart -
      help_timer->_ref_point.counterMS.QuadPart;

  
  
  ULARGE_INTEGER filetime_ref_as_ul;

  filetime_ref_as_ul.HighPart =
      help_timer->_ref_point.file_time.dwHighDateTime;
  filetime_ref_as_ul.LowPart =
      help_timer->_ref_point.file_time.dwLowDateTime;
  filetime_ref_as_ul.QuadPart +=
      (ULONGLONG)((elapsedMS.QuadPart)*1000*10);

  
  current_time.dwHighDateTime = filetime_ref_as_ul.HighPart;
  current_time.dwLowDateTime = filetime_ref_as_ul.LowPart;
  }

  
  class WindowsSystemClock : public RtpRtcpClock {
  public:
    WindowsSystemClock(WindowsHelpTimer* helpTimer)
      : _helpTimer(helpTimer) {}

    virtual ~WindowsSystemClock() {}

    virtual WebRtc_Word64 GetTimeInMS();

    virtual void CurrentNTP(WebRtc_UWord32& secs, WebRtc_UWord32& frac);

  private:
    WindowsHelpTimer* _helpTimer;
};

#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)


class UnixSystemClock : public RtpRtcpClock {
public:
  UnixSystemClock() {}
  virtual ~UnixSystemClock() {}

  virtual WebRtc_Word64 GetTimeInMS();

  virtual void CurrentNTP(WebRtc_UWord32& secs, WebRtc_UWord32& frac);
};
#endif

#if defined(_WIN32)
WebRtc_Word64 WindowsSystemClock::GetTimeInMS() {
  return TickTime::MillisecondTimestamp();
}



void WindowsSystemClock::CurrentNTP(WebRtc_UWord32& secs,
                                    WebRtc_UWord32& frac) {
  const WebRtc_UWord64 FILETIME_1970 = 0x019db1ded53e8000;

  FILETIME StartTime;
  WebRtc_UWord64 Time;
  struct timeval tv;

  
  
  get_time(_helpTimer, StartTime);

  Time = (((WebRtc_UWord64) StartTime.dwHighDateTime) << 32) +
         (WebRtc_UWord64) StartTime.dwLowDateTime;

  
  Time -= FILETIME_1970;

  tv.tv_sec = (WebRtc_UWord32)(Time / (WebRtc_UWord64)10000000);
  tv.tv_usec = (WebRtc_UWord32)((Time % (WebRtc_UWord64)10000000) / 10);

  double dtemp;

  secs = tv.tv_sec + NTP_JAN_1970;
  dtemp = tv.tv_usec / 1e6;

  if (dtemp >= 1) {
    dtemp -= 1;
    secs++;
  } else if (dtemp < -1) {
    dtemp += 1;
    secs--;
  }
  dtemp *= NTP_FRAC;
  frac = (WebRtc_UWord32)dtemp;
}

#elif ((defined WEBRTC_LINUX) || (defined WEBRTC_MAC))

WebRtc_Word64 UnixSystemClock::GetTimeInMS() {
  return TickTime::MillisecondTimestamp();
}


void UnixSystemClock::CurrentNTP(WebRtc_UWord32& secs, WebRtc_UWord32& frac) {
  double dtemp;
  struct timeval tv;
  struct timezone tz;
  tz.tz_minuteswest  = 0;
  tz.tz_dsttime = 0;
  gettimeofday(&tv, &tz);

  secs = tv.tv_sec + NTP_JAN_1970;
  dtemp = tv.tv_usec / 1e6;
  if (dtemp >= 1) {
    dtemp -= 1;
    secs++;
  } else if (dtemp < -1) {
    dtemp += 1;
    secs--;
  }
  dtemp *= NTP_FRAC;
  frac = (WebRtc_UWord32)dtemp;
}
#endif

#if defined(_WIN32)



static WindowsHelpTimer global_help_timer = {0, 0, {{ 0, 0}, 0}, 0};
#endif

RtpRtcpClock* GetSystemClock() {
#if defined(_WIN32)
  return new WindowsSystemClock(&global_help_timer);
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
  return new UnixSystemClock();
#else
  return NULL;
#endif
}

WebRtc_UWord32 GetCurrentRTP(RtpRtcpClock* clock, WebRtc_UWord32 freq) {
  const bool use_global_clock = (clock == NULL);
  RtpRtcpClock* local_clock = clock;
  if (use_global_clock) {
    local_clock = GetSystemClock();
  }
  WebRtc_UWord32 secs = 0, frac = 0;
  local_clock->CurrentNTP(secs, frac);
  if (use_global_clock) {
    delete local_clock;
  }
  return ConvertNTPTimeToRTP(secs, frac, freq);
}

WebRtc_UWord32 ConvertNTPTimeToRTP(WebRtc_UWord32 NTPsec,
                                   WebRtc_UWord32 NTPfrac,
                                   WebRtc_UWord32 freq) {
  float ftemp = (float)NTPfrac / (float)NTP_FRAC;
  WebRtc_UWord32 tmp = (WebRtc_UWord32)(ftemp * freq);
  return NTPsec * freq + tmp;
}

WebRtc_UWord32 ConvertNTPTimeToMS(WebRtc_UWord32 NTPsec,
                                  WebRtc_UWord32 NTPfrac) {
  int freq = 1000;
  float ftemp = (float)NTPfrac / (float)NTP_FRAC;
  WebRtc_UWord32 tmp = (WebRtc_UWord32)(ftemp * freq);
  WebRtc_UWord32 MStime = NTPsec * freq + tmp;
  return MStime;
}

bool OldTimestamp(uint32_t newTimestamp,
                  uint32_t existingTimestamp,
                  bool* wrapped) {
  bool tmpWrapped =
    (newTimestamp < 0x0000ffff && existingTimestamp > 0xffff0000) ||
    (newTimestamp > 0xffff0000 && existingTimestamp < 0x0000ffff);
  *wrapped = tmpWrapped;
  if (existingTimestamp > newTimestamp && !tmpWrapped) {
    return true;
  } else if (existingTimestamp <= newTimestamp && !tmpWrapped) {
    return false;
  } else if (existingTimestamp < newTimestamp && tmpWrapped) {
    return true;
  } else {
    return false;
  }
}





const WebRtc_UWord8* GetPayloadData(const WebRtcRTPHeader* rtp_header,
                                    const WebRtc_UWord8* packet) {
  return packet + rtp_header->header.headerLength;
}

WebRtc_UWord16 GetPayloadDataLength(const WebRtcRTPHeader* rtp_header,
                                    const WebRtc_UWord16 packet_length) {
  WebRtc_UWord16 length = packet_length - rtp_header->header.paddingLength -
      rtp_header->header.headerLength;
  return static_cast<WebRtc_UWord16>(length);
}

#if defined(_WIN32)
bool StringCompare(const char* str1, const char* str2,
                   const WebRtc_UWord32 length) {
  return (_strnicmp(str1, str2, length) == 0) ? true : false;
}
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
bool StringCompare(const char* str1, const char* str2,
                   const WebRtc_UWord32 length) {
  return (strncasecmp(str1, str2, length) == 0) ? true : false;
}
#endif

#if !defined(WEBRTC_LITTLE_ENDIAN) && !defined(WEBRTC_BIG_ENDIAN)
#error Either WEBRTC_LITTLE_ENDIAN or WEBRTC_BIG_ENDIAN must be defined
#endif





void AssignUWord32ToBuffer(WebRtc_UWord8* dataBuffer, WebRtc_UWord32 value) {
#if defined(WEBRTC_LITTLE_ENDIAN)
  dataBuffer[0] = static_cast<WebRtc_UWord8>(value >> 24);
  dataBuffer[1] = static_cast<WebRtc_UWord8>(value >> 16);
  dataBuffer[2] = static_cast<WebRtc_UWord8>(value >> 8);
  dataBuffer[3] = static_cast<WebRtc_UWord8>(value);
#else
  WebRtc_UWord32* ptr = reinterpret_cast<WebRtc_UWord32*>(dataBuffer);
  ptr[0] = value;
#endif
}

void AssignUWord24ToBuffer(WebRtc_UWord8* dataBuffer, WebRtc_UWord32 value) {
#if defined(WEBRTC_LITTLE_ENDIAN)
  dataBuffer[0] = static_cast<WebRtc_UWord8>(value >> 16);
  dataBuffer[1] = static_cast<WebRtc_UWord8>(value >> 8);
  dataBuffer[2] = static_cast<WebRtc_UWord8>(value);
#else
  dataBuffer[0] = static_cast<WebRtc_UWord8>(value);
  dataBuffer[1] = static_cast<WebRtc_UWord8>(value >> 8);
  dataBuffer[2] = static_cast<WebRtc_UWord8>(value >> 16);
#endif
}

void AssignUWord16ToBuffer(WebRtc_UWord8* dataBuffer, WebRtc_UWord16 value) {
#if defined(WEBRTC_LITTLE_ENDIAN)
  dataBuffer[0] = static_cast<WebRtc_UWord8>(value >> 8);
  dataBuffer[1] = static_cast<WebRtc_UWord8>(value);
#else
  WebRtc_UWord16* ptr = reinterpret_cast<WebRtc_UWord16*>(dataBuffer);
  ptr[0] = value;
#endif
}

WebRtc_UWord16 BufferToUWord16(const WebRtc_UWord8* dataBuffer) {
#if defined(WEBRTC_LITTLE_ENDIAN)
  return (dataBuffer[0] << 8) + dataBuffer[1];
#else
  return *reinterpret_cast<const WebRtc_UWord16*>(dataBuffer);
#endif
}

WebRtc_UWord32 BufferToUWord24(const WebRtc_UWord8* dataBuffer) {
  return (dataBuffer[0] << 16) + (dataBuffer[1] << 8) + dataBuffer[2];
}

WebRtc_UWord32 BufferToUWord32(const WebRtc_UWord8* dataBuffer) {
#if defined(WEBRTC_LITTLE_ENDIAN)
  return (dataBuffer[0] << 24) + (dataBuffer[1] << 16) + (dataBuffer[2] << 8) +
      dataBuffer[3];
#else
  return *reinterpret_cast<const WebRtc_UWord32*>(dataBuffer);
#endif
}

WebRtc_UWord32 pow2(WebRtc_UWord8 exp) {
  return 1 << exp;
}

void RTPPayload::SetType(RtpVideoCodecTypes videoType) {
  type = videoType;

  switch (type) {
    case kRtpNoVideo:
      break;
    case kRtpVp8Video: {
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

RTPHeaderParser::RTPHeaderParser(const WebRtc_UWord8* rtpData,
                                 const WebRtc_UWord32 rtpDataLength)
  : _ptrRTPDataBegin(rtpData),
    _ptrRTPDataEnd(rtpData ? (rtpData + rtpDataLength) : NULL) {
}

RTPHeaderParser::~RTPHeaderParser() {
}

bool RTPHeaderParser::RTCP() const {
  
  
  
  
  
















  






  










  const WebRtc_UWord8  payloadType = _ptrRTPDataBegin[1];

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

bool RTPHeaderParser::Parse(WebRtcRTPHeader& parsedPacket,
                            RtpHeaderExtensionMap* ptrExtensionMap) const {
  const ptrdiff_t length = _ptrRTPDataEnd - _ptrRTPDataBegin;

  if (length < 12) {
    return false;
  }

  
  const WebRtc_UWord8 V  = _ptrRTPDataBegin[0] >> 6;
  
  const bool          P  = ((_ptrRTPDataBegin[0] & 0x20) == 0) ? false : true;
  
  const bool          X  = ((_ptrRTPDataBegin[0] & 0x10) == 0) ? false : true;
  const WebRtc_UWord8 CC = _ptrRTPDataBegin[0] & 0x0f;
  const bool          M  = ((_ptrRTPDataBegin[1] & 0x80) == 0) ? false : true;

  const WebRtc_UWord8 PT = _ptrRTPDataBegin[1] & 0x7f;

  const WebRtc_UWord16 sequenceNumber = (_ptrRTPDataBegin[2] << 8) +
      _ptrRTPDataBegin[3];

  const WebRtc_UWord8* ptr = &_ptrRTPDataBegin[4];

  WebRtc_UWord32 RTPTimestamp = *ptr++ << 24;
  RTPTimestamp += *ptr++ << 16;
  RTPTimestamp += *ptr++ << 8;
  RTPTimestamp += *ptr++;

  WebRtc_UWord32 SSRC = *ptr++ << 24;
  SSRC += *ptr++ << 16;
  SSRC += *ptr++ << 8;
  SSRC += *ptr++;

  if (V != 2) {
    return false;
  }

  const WebRtc_UWord8 CSRCocts = CC * 4;

  if ((ptr + CSRCocts) > _ptrRTPDataEnd) {
    return false;
  }

  parsedPacket.header.markerBit      = M;
  parsedPacket.header.payloadType    = PT;
  parsedPacket.header.sequenceNumber = sequenceNumber;
  parsedPacket.header.timestamp      = RTPTimestamp;
  parsedPacket.header.ssrc           = SSRC;
  parsedPacket.header.numCSRCs       = CC;
  parsedPacket.header.paddingLength  = P ? *(_ptrRTPDataEnd - 1) : 0;

  for (unsigned int i = 0; i < CC; ++i) {
    WebRtc_UWord32 CSRC = *ptr++ << 24;
    CSRC += *ptr++ << 16;
    CSRC += *ptr++ << 8;
    CSRC += *ptr++;
    parsedPacket.header.arrOfCSRCs[i] = CSRC;
  }
  parsedPacket.type.Audio.numEnergy = parsedPacket.header.numCSRCs;

  parsedPacket.header.headerLength   = 12 + CSRCocts;

  
  
  parsedPacket.extension.transmissionTimeOffset = 0;

  if (X) {
    








    const ptrdiff_t remain = _ptrRTPDataEnd - ptr;
    if (remain < 4) {
      return false;
    }

    parsedPacket.header.headerLength += 4;

    WebRtc_UWord16 definedByProfile = *ptr++ << 8;
    definedByProfile += *ptr++;

    WebRtc_UWord16 XLen = *ptr++ << 8;
    XLen += *ptr++; 
    XLen *= 4; 

    if (remain < (4 + XLen)) {
      return false;
    }
    if (definedByProfile == RTP_ONE_BYTE_HEADER_EXTENSION) {
      const WebRtc_UWord8* ptrRTPDataExtensionEnd = ptr + XLen;
      ParseOneByteExtensionHeader(parsedPacket,
                                  ptrExtensionMap,
                                  ptrRTPDataExtensionEnd,
                                  ptr);
    }
    parsedPacket.header.headerLength += XLen;
  }
  return true;
}

void RTPHeaderParser::ParseOneByteExtensionHeader(
    WebRtcRTPHeader& parsedPacket,
    const RtpHeaderExtensionMap* ptrExtensionMap,
    const WebRtc_UWord8* ptrRTPDataExtensionEnd,
    const WebRtc_UWord8* ptr) const {
  if (!ptrExtensionMap) {
    return;
  }

  while (ptrRTPDataExtensionEnd - ptr > 0) {
    
    
    
    
    

    const WebRtc_UWord8 id = (*ptr & 0xf0) >> 4;
    const WebRtc_UWord8 len = (*ptr & 0x0f);
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
        
        
        
        
        

        WebRtc_Word32 transmissionTimeOffset = *ptr++ << 16;
        transmissionTimeOffset += *ptr++ << 8;
        transmissionTimeOffset += *ptr++;
        parsedPacket.extension.transmissionTimeOffset = transmissionTimeOffset;
        if (transmissionTimeOffset & 0x800000) {
          
          parsedPacket.extension.transmissionTimeOffset |= 0xFF000000;
        }
        break;
      }
      case kRtpExtensionAudioLevel: {
        
        
        
        
        
        
        

        
        
        
        
        
        break;
      }
      default: {
        WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
                     "Extension type not implemented.");
        return;
      }
    }
    WebRtc_UWord8 num_bytes = ParsePaddingBytes(ptrRTPDataExtensionEnd, ptr);
    ptr += num_bytes;
  }
}

WebRtc_UWord8 RTPHeaderParser::ParsePaddingBytes(
  const WebRtc_UWord8* ptrRTPDataExtensionEnd,
  const WebRtc_UWord8* ptr) const {

  WebRtc_UWord8 num_zero_bytes = 0;
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
                                   const WebRtc_UWord8* payloadData,
                                   WebRtc_UWord16 payloadDataLength,
                                   WebRtc_Word32 id)
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
    case kRtpNoVideo:
      return ParseGeneric(parsedPacket);
    case kRtpVp8Video:
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
  const WebRtc_UWord8* dataPtr = _dataPtr;
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
                                        const WebRtc_UWord8* dataPtr,
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
                                        const WebRtc_UWord8* dataPtr,
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
                                        const WebRtc_UWord8** dataPtr,
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
                                        const WebRtc_UWord8** dataPtr,
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
                                           const WebRtc_UWord8** dataPtr,
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
