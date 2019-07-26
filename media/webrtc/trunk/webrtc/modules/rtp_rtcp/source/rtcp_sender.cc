









#include "rtcp_sender.h"

#include <cassert>  
#include <cstdlib>  
#include <string.h>  

#include "common_types.h"
#include "modules/rtp_rtcp/source/rtp_rtcp_impl.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {

using RTCPUtility::RTCPCnameInformation;

RTCPSender::RTCPSender(const WebRtc_Word32 id,
                       const bool audio,
                       RtpRtcpClock* clock,
                       ModuleRtpRtcpImpl* owner) :
    _id(id),
    _audio(audio),
    _clock(*clock),
    _method(kRtcpOff),
    _rtpRtcp(*owner),
    _criticalSectionTransport(CriticalSectionWrapper::CreateCriticalSection()),
    _cbTransport(NULL),

    _criticalSectionRTCPSender(CriticalSectionWrapper::CreateCriticalSection()),
    _usingNack(false),
    _sending(false),
    _sendTMMBN(false),
    _REMB(false),
    _sendREMB(false),
    _TMMBR(false),
    _IJ(false),
    _nextTimeToSendRTCP(0),
    start_timestamp_(0),
    last_rtp_timestamp_(0),
    last_frame_capture_time_ms_(-1),
    _SSRC(0),
    _remoteSSRC(0),
    _CNAME(),
    _reportBlocks(),
    _csrcCNAMEs(),

    _cameraDelayMS(0),

    _lastSendReport(),
    _lastRTCPTime(),

    _CSRCs(0),
    _CSRC(),
    _includeCSRCs(true),

    _sequenceNumberFIR(0),

    _lengthRembSSRC(0),
    _sizeRembSSRC(0),
    _rembSSRC(NULL),
    _rembBitrate(0),

    _tmmbrHelp(),
    _tmmbr_Send(0),
    _packetOH_Send(0),

    _appSend(false),
    _appSubType(0),
    _appName(),
    _appData(NULL),
    _appLength(0),
    _xrSendVoIPMetric(false),
    _xrVoIPMetric()
{
    memset(_CNAME, 0, sizeof(_CNAME));
    memset(_lastSendReport, 0, sizeof(_lastSendReport));
    memset(_lastRTCPTime, 0, sizeof(_lastRTCPTime));

    WEBRTC_TRACE(kTraceMemory, kTraceRtpRtcp, id, "%s created", __FUNCTION__);
}

RTCPSender::~RTCPSender() {
  delete [] _rembSSRC;
  delete [] _appData;

  while (!_reportBlocks.empty()) {
    std::map<WebRtc_UWord32, RTCPReportBlock*>::iterator it =
        _reportBlocks.begin();
    delete it->second;
    _reportBlocks.erase(it);
  }
  while (!_csrcCNAMEs.empty()) {
    std::map<WebRtc_UWord32, RTCPCnameInformation*>::iterator it =
        _csrcCNAMEs.begin();
    delete it->second;
    _csrcCNAMEs.erase(it);
  }
  delete _criticalSectionTransport;
  delete _criticalSectionRTCPSender;

  WEBRTC_TRACE(kTraceMemory, kTraceRtpRtcp, _id, "%s deleted", __FUNCTION__);
}

WebRtc_Word32
RTCPSender::Init()
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    _method = kRtcpOff;
    _cbTransport = NULL;
    _usingNack = false;
    _sending = false;
    _sendTMMBN = false;
    _TMMBR = false;
    _IJ = false;
    _REMB = false;
    _sendREMB = false;
    last_rtp_timestamp_ = 0;
    last_frame_capture_time_ms_ = -1;
    start_timestamp_ = -1;
    _SSRC = 0;
    _remoteSSRC = 0;
    _cameraDelayMS = 0;
    _sequenceNumberFIR = 0;
    _tmmbr_Send = 0;
    _packetOH_Send = 0;
    _nextTimeToSendRTCP = 0;
    _CSRCs = 0;
    _appSend = false;
    _appSubType = 0;

    if(_appData)
    {
        delete [] _appData;
        _appData = NULL;
    }
    _appLength = 0;

    _xrSendVoIPMetric = false;

    memset(&_xrVoIPMetric, 0, sizeof(_xrVoIPMetric));
    memset(_CNAME, 0, sizeof(_CNAME));
    memset(_lastSendReport, 0, sizeof(_lastSendReport));
    memset(_lastRTCPTime, 0, sizeof(_lastRTCPTime));
    return 0;
}

void
RTCPSender::ChangeUniqueId(const WebRtc_Word32 id)
{
    _id = id;
}

WebRtc_Word32
RTCPSender::RegisterSendTransport(Transport* outgoingTransport)
{
    CriticalSectionScoped lock(_criticalSectionTransport);
    _cbTransport = outgoingTransport;
    return 0;
}

RTCPMethod
RTCPSender::Status() const
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    return _method;
}

WebRtc_Word32
RTCPSender::SetRTCPStatus(const RTCPMethod method)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    if(method != kRtcpOff)
    {
        if(_audio)
        {
            _nextTimeToSendRTCP = _clock.GetTimeInMS() + (RTCP_INTERVAL_AUDIO_MS/2);
        } else
        {
            _nextTimeToSendRTCP = _clock.GetTimeInMS() + (RTCP_INTERVAL_VIDEO_MS/2);
        }
    }
    _method = method;
    return 0;
}

bool
RTCPSender::Sending() const
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    return _sending;
}

WebRtc_Word32
RTCPSender::SetSendingStatus(const bool sending)
{
    bool sendRTCPBye = false;
    {
        CriticalSectionScoped lock(_criticalSectionRTCPSender);

        if(_method != kRtcpOff)
        {
            if(sending == false && _sending == true)
            {
                
                sendRTCPBye = true;
            }
        }
        _sending = sending;
    }
    if(sendRTCPBye)
    {
        return SendRTCP(kRtcpBye);
    }
    return 0;
}

bool
RTCPSender::REMB() const
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    return _REMB;
}

WebRtc_Word32
RTCPSender::SetREMBStatus(const bool enable)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _REMB = enable;
    return 0;
}

WebRtc_Word32
RTCPSender::SetREMBData(const WebRtc_UWord32 bitrate,
                        const WebRtc_UWord8 numberOfSSRC,
                        const WebRtc_UWord32* SSRC)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _rembBitrate = bitrate;
 
    if(_sizeRembSSRC < numberOfSSRC)
    {
        delete [] _rembSSRC;
        _rembSSRC = new WebRtc_UWord32[numberOfSSRC];
        _sizeRembSSRC = numberOfSSRC;
    } 

    _lengthRembSSRC = numberOfSSRC;
    for (int i = 0; i < numberOfSSRC; i++)
    {  
        _rembSSRC[i] = SSRC[i];
    }
    _sendREMB = true;
    return 0;
}

bool
RTCPSender::TMMBR() const
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    return _TMMBR;
}

WebRtc_Word32
RTCPSender::SetTMMBRStatus(const bool enable)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _TMMBR = enable;
    return 0;
}

bool
RTCPSender::IJ() const
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    return _IJ;
}

WebRtc_Word32
RTCPSender::SetIJStatus(const bool enable)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _IJ = enable;
    return 0;
}

void RTCPSender::SetStartTimestamp(uint32_t start_timestamp) {
  start_timestamp_ = start_timestamp;
}

void RTCPSender::SetLastRtpTime(uint32_t rtp_timestamp,
                                int64_t capture_time_ms) {
  last_rtp_timestamp_ = rtp_timestamp;
  if (capture_time_ms < 0) {
    
    last_frame_capture_time_ms_ = _clock.GetTimeInMS();
  } else {
    last_frame_capture_time_ms_ = capture_time_ms;
  }
}

void
RTCPSender::SetSSRC( const WebRtc_UWord32 ssrc)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    if(_SSRC != 0)
    {
        
        
        
        _nextTimeToSendRTCP = _clock.GetTimeInMS() + 100;
    }
    _SSRC = ssrc;
}

WebRtc_Word32
RTCPSender::SetRemoteSSRC( const WebRtc_UWord32 ssrc)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _remoteSSRC = ssrc;
    return 0;
}

WebRtc_Word32
RTCPSender::SetCameraDelay(const WebRtc_Word32 delayMS)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    if(delayMS > 1000 || delayMS < -1000)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument, delay can't be larger than 1 sec", __FUNCTION__);
        return -1;
    }
    _cameraDelayMS = delayMS;
    return 0;
}

WebRtc_Word32 RTCPSender::CNAME(char cName[RTCP_CNAME_SIZE]) {
  assert(cName);
  CriticalSectionScoped lock(_criticalSectionRTCPSender);
  cName[RTCP_CNAME_SIZE - 1] = 0;
  strncpy(cName, _CNAME, RTCP_CNAME_SIZE - 1);
  return 0;
}

WebRtc_Word32 RTCPSender::SetCNAME(const char cName[RTCP_CNAME_SIZE]) {
  if (!cName)
    return -1;

  CriticalSectionScoped lock(_criticalSectionRTCPSender);
  _CNAME[RTCP_CNAME_SIZE - 1] = 0;
  strncpy(_CNAME, cName, RTCP_CNAME_SIZE - 1);
  return 0;
}

WebRtc_Word32 RTCPSender::AddMixedCNAME(const WebRtc_UWord32 SSRC,
                                        const char cName[RTCP_CNAME_SIZE]) {
  assert(cName);
  CriticalSectionScoped lock(_criticalSectionRTCPSender);
  if (_csrcCNAMEs.size() >= kRtpCsrcSize) {
    return -1;
  }
  RTCPCnameInformation* ptr = new RTCPCnameInformation();
  ptr->name[RTCP_CNAME_SIZE - 1] = 0;
  strncpy(ptr->name, cName, RTCP_CNAME_SIZE - 1);
  _csrcCNAMEs[SSRC] = ptr;
  return 0;
}

WebRtc_Word32 RTCPSender::RemoveMixedCNAME(const WebRtc_UWord32 SSRC) {
  CriticalSectionScoped lock(_criticalSectionRTCPSender);
  std::map<WebRtc_UWord32, RTCPCnameInformation*>::iterator it =
      _csrcCNAMEs.find(SSRC);

  if (it == _csrcCNAMEs.end()) {
    return -1;
  }
  delete it->second;
  _csrcCNAMEs.erase(it);
  return 0;
}

bool
RTCPSender::TimeToSendRTCPReport(const bool sendKeyframeBeforeRTP) const
{

























































    WebRtc_Word64 now = _clock.GetTimeInMS();

    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    if(_method == kRtcpOff)
    {
        return false;
    }

    if(!_audio && sendKeyframeBeforeRTP)
    {
        
        
        now += RTCP_SEND_BEFORE_KEY_FRAME_MS;
    }

    if(now > _nextTimeToSendRTCP)
    {
        return true;

    } else if(now < 0x0000ffff && _nextTimeToSendRTCP > 0xffff0000) 
    {
        
        return true;
    }
    return false;
}

WebRtc_UWord32
RTCPSender::LastSendReport( WebRtc_UWord32& lastRTCPTime)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    lastRTCPTime = _lastRTCPTime[0];
    return _lastSendReport[0];
}

WebRtc_UWord32
RTCPSender::SendTimeOfSendReport(const WebRtc_UWord32 sendReport)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    
    if((_lastSendReport[0] == 0) || (sendReport == 0))
    {
        return 0; 
    } else
    {
        for(int i = 0; i < RTCP_NUMBER_OF_SR; ++i)
        {
            if( _lastSendReport[i] == sendReport)
            {
                return _lastRTCPTime[i];
            }
        }
    }
    return 0;
}

WebRtc_Word32 RTCPSender::AddReportBlock(const WebRtc_UWord32 SSRC,
                                         const RTCPReportBlock* reportBlock) {
  if (reportBlock == NULL) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s invalid argument", __FUNCTION__);
    return -1;
  }
  CriticalSectionScoped lock(_criticalSectionRTCPSender);

  if (_reportBlocks.size() >= RTCP_MAX_REPORT_BLOCKS) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s invalid argument", __FUNCTION__);
    return -1;
  }
  std::map<WebRtc_UWord32, RTCPReportBlock*>::iterator it =
      _reportBlocks.find(SSRC);
  if (it != _reportBlocks.end()) {
    delete it->second;
    _reportBlocks.erase(it);
  }
  RTCPReportBlock* copyReportBlock = new RTCPReportBlock();
  memcpy(copyReportBlock, reportBlock, sizeof(RTCPReportBlock));
  _reportBlocks[SSRC] = copyReportBlock;
  return 0;
}

WebRtc_Word32 RTCPSender::RemoveReportBlock(const WebRtc_UWord32 SSRC) {
  CriticalSectionScoped lock(_criticalSectionRTCPSender);

  std::map<WebRtc_UWord32, RTCPReportBlock*>::iterator it =
      _reportBlocks.find(SSRC);

  if (it == _reportBlocks.end()) {
    return -1;
  }
  delete it->second;
  _reportBlocks.erase(it);
  return 0;
}

WebRtc_Word32
RTCPSender::BuildSR(WebRtc_UWord8* rtcpbuffer,
                    WebRtc_UWord32& pos,
                    const WebRtc_UWord32 NTPsec,
                    const WebRtc_UWord32 NTPfrac,
                    const RTCPReportBlock* received)
{
    
    if(pos + 52 >= IP_PACKET_SIZE)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument", __FUNCTION__);
        return -2;
    }
    WebRtc_UWord32 RTPtime;

    WebRtc_UWord32 posNumberOfReportBlocks = pos;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80;

    
    rtcpbuffer[pos++]=(WebRtc_UWord8)200;

    for(int i = (RTCP_NUMBER_OF_SR-2); i >= 0; i--)
    {
        
        _lastSendReport[i+1] = _lastSendReport[i];
        _lastRTCPTime[i+1] =_lastRTCPTime[i];
    }

    _lastRTCPTime[0] = ModuleRTPUtility::ConvertNTPTimeToMS(NTPsec, NTPfrac);
    _lastSendReport[0] = (NTPsec << 16) + (NTPfrac >> 16);

    WebRtc_UWord32 freqHz = 90000; 
    if(_audio) {
      freqHz =  _rtpRtcp.CurrentSendFrequencyHz();
    }
    
    
    
    
    RTPtime = start_timestamp_ + last_rtp_timestamp_ + (_clock.GetTimeInMS() -
        last_frame_capture_time_ms_) * (freqHz / 1000);

    
    
    pos++;
    pos++;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;
    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, NTPsec);
    pos += 4;
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, NTPfrac);
    pos += 4;
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, RTPtime);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _rtpRtcp.PacketCountSent());
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _rtpRtcp.ByteCountSent());
    pos += 4;

    WebRtc_UWord8 numberOfReportBlocks = 0;
    WebRtc_Word32 retVal = AddReportBlocks(rtcpbuffer, pos, numberOfReportBlocks, received, NTPsec, NTPfrac);
    if(retVal < 0)
    {
        
        return retVal ;
    }
    rtcpbuffer[posNumberOfReportBlocks] += numberOfReportBlocks;

    WebRtc_UWord16 len = WebRtc_UWord16((pos/4) -1);
    ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer+2, len);
    return 0;
}


WebRtc_Word32 RTCPSender::BuildSDEC(WebRtc_UWord8* rtcpbuffer,
                                    WebRtc_UWord32& pos) {
  size_t lengthCname = strlen(_CNAME);
  assert(lengthCname < RTCP_CNAME_SIZE);

  
  if(pos + 12 + lengthCname  >= IP_PACKET_SIZE) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s invalid argument", __FUNCTION__);
    return -2;
  }
  

  
  rtcpbuffer[pos++] = static_cast<WebRtc_UWord8>(0x80 + 1 + _csrcCNAMEs.size());
  rtcpbuffer[pos++] = static_cast<WebRtc_UWord8>(202);

  
  WebRtc_UWord32 SDESLengthPos = pos;
  pos++;
  pos++;

  
  ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
  pos += 4;

  
  rtcpbuffer[pos++] = static_cast<WebRtc_UWord8>(1);

  
  rtcpbuffer[pos++] = static_cast<WebRtc_UWord8>(lengthCname);

  WebRtc_UWord16 SDESLength = 10;

  memcpy(&rtcpbuffer[pos], _CNAME, lengthCname);
  pos += lengthCname;
  SDESLength += (WebRtc_UWord16)lengthCname;

  WebRtc_UWord16 padding = 0;
  
  if ((pos % 4) == 0) {
    padding++;
    rtcpbuffer[pos++]=0;
  }
  while ((pos % 4) != 0) {
    padding++;
    rtcpbuffer[pos++]=0;
  }
  SDESLength += padding;

  std::map<WebRtc_UWord32, RTCPUtility::RTCPCnameInformation*>::iterator it =
      _csrcCNAMEs.begin();

  for(; it != _csrcCNAMEs.end(); it++) {
    RTCPCnameInformation* cname = it->second;
    WebRtc_UWord32 SSRC = it->first;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, SSRC);
    pos += 4;

    
    rtcpbuffer[pos++] = static_cast<WebRtc_UWord8>(1);

    size_t length = strlen(cname->name);
    assert(length < RTCP_CNAME_SIZE);

    rtcpbuffer[pos++]= static_cast<WebRtc_UWord8>(length);
    SDESLength += 6;

    memcpy(&rtcpbuffer[pos],cname->name, length);

    pos += length;
    SDESLength += length;
    WebRtc_UWord16 padding = 0;

    
    if((pos % 4) == 0){
      padding++;
      rtcpbuffer[pos++]=0;
    }
    while((pos % 4) != 0){
      padding++;
      rtcpbuffer[pos++] = 0;
    }
    SDESLength += padding;
  }
  
  WebRtc_UWord16 buffer_length = (SDESLength / 4) - 1;
  ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer + SDESLengthPos,
                                          buffer_length);
  return 0;
}

WebRtc_Word32
RTCPSender::BuildRR(WebRtc_UWord8* rtcpbuffer,
                    WebRtc_UWord32& pos,
                    const WebRtc_UWord32 NTPsec,
                    const WebRtc_UWord32 NTPfrac,
                    const RTCPReportBlock* received)
{
    
    if(pos + 32 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    WebRtc_UWord32 posNumberOfReportBlocks = pos;

    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80;
    rtcpbuffer[pos++]=(WebRtc_UWord8)201;

    
    pos++;
    pos++;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    WebRtc_UWord8 numberOfReportBlocks = 0;
    WebRtc_Word32 retVal = AddReportBlocks(rtcpbuffer, pos, numberOfReportBlocks, received, NTPsec, NTPfrac);
    if(retVal < 0)
    {
        return retVal;
    }
    rtcpbuffer[posNumberOfReportBlocks] += numberOfReportBlocks;

    WebRtc_UWord16 len = WebRtc_UWord16((pos)/4 -1);
    ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer+2, len);
    return 0;
}



















WebRtc_Word32
RTCPSender::BuildExtendedJitterReport(
    WebRtc_UWord8* rtcpbuffer,
    WebRtc_UWord32& pos,
    const WebRtc_UWord32 jitterTransmissionTimeOffset)
{
    if (_reportBlocks.size() > 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id, "Not implemented.");
        return 0;
    }

    
    if(pos + 8 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    WebRtc_UWord8 RC = 1;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + RC;
    rtcpbuffer[pos++]=(WebRtc_UWord8)195;

    
    rtcpbuffer[pos++]=(WebRtc_UWord8)0;
    rtcpbuffer[pos++]=(WebRtc_UWord8)(1);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer + pos,
                                            jitterTransmissionTimeOffset);
    pos += 4;
    return 0;
}

WebRtc_Word32
RTCPSender::BuildPLI(WebRtc_UWord8* rtcpbuffer, WebRtc_UWord32& pos)
{
    
    if(pos + 12 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    WebRtc_UWord8 FMT = 1;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + FMT;
    rtcpbuffer[pos++]=(WebRtc_UWord8)206;

    
    rtcpbuffer[pos++]=(WebRtc_UWord8)0;
    rtcpbuffer[pos++]=(WebRtc_UWord8)(2);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;
    return 0;
}

WebRtc_Word32 RTCPSender::BuildFIR(WebRtc_UWord8* rtcpbuffer,
                                   WebRtc_UWord32& pos,
                                   bool repeat) {
  
  if(pos + 20 >= IP_PACKET_SIZE)  {
    return -2;
  }
  if (!repeat) {
    _sequenceNumberFIR++;   
  }

  
  WebRtc_UWord8 FMT = 4;
  rtcpbuffer[pos++] = (WebRtc_UWord8)0x80 + FMT;
  rtcpbuffer[pos++] = (WebRtc_UWord8)206;

  
  rtcpbuffer[pos++] = (WebRtc_UWord8)0;
  rtcpbuffer[pos++] = (WebRtc_UWord8)(4);

  
  ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer + pos, _SSRC);
  pos += 4;

  
  
  rtcpbuffer[pos++] = (WebRtc_UWord8)0;
  rtcpbuffer[pos++] = (WebRtc_UWord8)0;
  rtcpbuffer[pos++] = (WebRtc_UWord8)0;
  rtcpbuffer[pos++] = (WebRtc_UWord8)0;

  
  ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer + pos, _remoteSSRC);
  pos += 4;

  rtcpbuffer[pos++] = (WebRtc_UWord8)(_sequenceNumberFIR);
  rtcpbuffer[pos++] = (WebRtc_UWord8)0;
  rtcpbuffer[pos++] = (WebRtc_UWord8)0;
  rtcpbuffer[pos++] = (WebRtc_UWord8)0;
  return 0;
}








WebRtc_Word32
RTCPSender::BuildSLI(WebRtc_UWord8* rtcpbuffer, WebRtc_UWord32& pos, const WebRtc_UWord8 pictureID)
{
    
    if(pos + 16 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    WebRtc_UWord8 FMT = 2;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + FMT;
    rtcpbuffer[pos++]=(WebRtc_UWord8)206;

    
    rtcpbuffer[pos++]=(WebRtc_UWord8)0;
    rtcpbuffer[pos++]=(WebRtc_UWord8)(3);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;

    
    
    
    WebRtc_UWord32 sliField = (0x1fff << 6)+ (0x3f & pictureID);
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, sliField);
    pos += 4;
    return 0;
}













WebRtc_Word32
RTCPSender::BuildRPSI(WebRtc_UWord8* rtcpbuffer,
                     WebRtc_UWord32& pos,
                     const WebRtc_UWord64 pictureID,
                     const WebRtc_UWord8 payloadType)
{
    
    if(pos + 24 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    WebRtc_UWord8 FMT = 3;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + FMT;
    rtcpbuffer[pos++]=(WebRtc_UWord8)206;

    
    WebRtc_UWord32 bitsRequired = 7;
    WebRtc_UWord8 bytesRequired = 1;
    while((pictureID>>bitsRequired) > 0)
    {
        bitsRequired += 7;
        bytesRequired++;
    }

    WebRtc_UWord8 size = 3;
    if(bytesRequired > 6)
    {
        size = 5;
    } else if(bytesRequired > 2)
    {
        size = 4;
    }
    rtcpbuffer[pos++]=(WebRtc_UWord8)0;
    rtcpbuffer[pos++]=size;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;

    
    WebRtc_UWord8 paddingBytes = 4-((2+bytesRequired)%4);
    if(paddingBytes == 4)
    {
        paddingBytes = 0;
    }
    
    rtcpbuffer[pos] = paddingBytes*8; 
    pos++;

    
    rtcpbuffer[pos] = payloadType;
    pos++;

    
    for(int i = bytesRequired-1; i > 0; i--)
    {
        rtcpbuffer[pos] = 0x80 | WebRtc_UWord8(pictureID >> (i*7));
        pos++;
    }
    
    rtcpbuffer[pos] = WebRtc_UWord8(pictureID & 0x7f);
    pos++;

    
    for(int j = 0; j <paddingBytes; j++)
    {
        rtcpbuffer[pos] = 0;
        pos++;
    }
    return 0;
}

WebRtc_Word32
RTCPSender::BuildREMB(WebRtc_UWord8* rtcpbuffer, WebRtc_UWord32& pos)
{
    
    if(pos + 20 + 4 * _lengthRembSSRC >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    WebRtc_UWord8 FMT = 15;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + FMT;
    rtcpbuffer[pos++]=(WebRtc_UWord8)206;

    rtcpbuffer[pos++]=(WebRtc_UWord8)0;
    rtcpbuffer[pos++]=_lengthRembSSRC + 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, 0);
    pos += 4;

    rtcpbuffer[pos++]='R';
    rtcpbuffer[pos++]='E';
    rtcpbuffer[pos++]='M';
    rtcpbuffer[pos++]='B';

    rtcpbuffer[pos++] = _lengthRembSSRC;
    
    
    WebRtc_UWord8 brExp = 0;
    for(WebRtc_UWord32 i=0; i<64; i++)
    {
        if(_rembBitrate <= ((WebRtc_UWord32)262143 << i))
        {
            brExp = i;
            break;
        }
    }
    const WebRtc_UWord32 brMantissa = (_rembBitrate >> brExp);
    rtcpbuffer[pos++]=(WebRtc_UWord8)((brExp << 2) + ((brMantissa >> 16) & 0x03));
    rtcpbuffer[pos++]=(WebRtc_UWord8)(brMantissa >> 8);
    rtcpbuffer[pos++]=(WebRtc_UWord8)(brMantissa);

    for (int i = 0; i < _lengthRembSSRC; i++) 
    { 
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _rembSSRC[i]);
        pos += 4;
    }
    return 0;
}

void
RTCPSender::SetTargetBitrate(unsigned int target_bitrate)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _tmmbr_Send = target_bitrate / 1000;
}

WebRtc_Word32
RTCPSender::BuildTMMBR(WebRtc_UWord8* rtcpbuffer, WebRtc_UWord32& pos)
{
    
    
    

    
    bool tmmbrOwner = false;
    
    TMMBRSet* candidateSet = _tmmbrHelp.CandidateSet();

    
    
    
    WebRtc_Word32 lengthOfBoundingSet
        = _rtpRtcp.BoundingSet(tmmbrOwner, candidateSet);

    if(lengthOfBoundingSet > 0)
    {
        for (WebRtc_Word32 i = 0; i < lengthOfBoundingSet; i++)
        {
            if( candidateSet->Tmmbr(i) == _tmmbr_Send &&
                candidateSet->PacketOH(i) == _packetOH_Send)
            {
                
                return 0;
            }
        }
        if(!tmmbrOwner)
        {
            
            
            candidateSet->SetEntry(lengthOfBoundingSet,
                                   _tmmbr_Send,
                                   _packetOH_Send,
                                   _SSRC);
            int numCandidates = lengthOfBoundingSet+ 1;

            
            TMMBRSet* boundingSet = NULL;
            int numBoundingSet = _tmmbrHelp.FindTMMBRBoundingSet(boundingSet);
            if(numBoundingSet > 0 || numBoundingSet <= numCandidates)
            {
                tmmbrOwner = _tmmbrHelp.IsOwner(_SSRC, numBoundingSet);
            }
            if(!tmmbrOwner)
            {
                
                return 0;
            }
        }
    }

    if(_tmmbr_Send)
    {
        
        if(pos + 20 >= IP_PACKET_SIZE)
        {
            return -2;
        }
        
        WebRtc_UWord8 FMT = 3;
        rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + FMT;
        rtcpbuffer[pos++]=(WebRtc_UWord8)205;

        
        rtcpbuffer[pos++]=(WebRtc_UWord8)0;
        rtcpbuffer[pos++]=(WebRtc_UWord8)(4);

        
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
        pos += 4;

        

        
        rtcpbuffer[pos++]=(WebRtc_UWord8)0;
        rtcpbuffer[pos++]=(WebRtc_UWord8)0;
        rtcpbuffer[pos++]=(WebRtc_UWord8)0;
        rtcpbuffer[pos++]=(WebRtc_UWord8)0;

        
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
        pos += 4;

        WebRtc_UWord32 bitRate = _tmmbr_Send*1000;
        WebRtc_UWord32 mmbrExp = 0;
        for(WebRtc_UWord32 i=0;i<64;i++)
        {
            if(bitRate <= ((WebRtc_UWord32)131071 << i))
            {
                mmbrExp = i;
                break;
            }
        }
        WebRtc_UWord32 mmbrMantissa = (bitRate >> mmbrExp);

        rtcpbuffer[pos++]=(WebRtc_UWord8)((mmbrExp << 2) + ((mmbrMantissa >> 15) & 0x03));
        rtcpbuffer[pos++]=(WebRtc_UWord8)(mmbrMantissa >> 7);
        rtcpbuffer[pos++]=(WebRtc_UWord8)((mmbrMantissa << 1) + ((_packetOH_Send >> 8)& 0x01));
        rtcpbuffer[pos++]=(WebRtc_UWord8)(_packetOH_Send);
    }
    return 0;
}

WebRtc_Word32
RTCPSender::BuildTMMBN(WebRtc_UWord8* rtcpbuffer, WebRtc_UWord32& pos)
{
    TMMBRSet* boundingSet = _tmmbrHelp.BoundingSetToSend();
    if(boundingSet == NULL)
    {
        return -1;
    }
    
    if(pos + 12 + boundingSet->lengthOfSet()*8 >= IP_PACKET_SIZE)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument", __FUNCTION__);
        return -2;
    }
    WebRtc_UWord8 FMT = 4;
    
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + FMT;
    rtcpbuffer[pos++]=(WebRtc_UWord8)205;

    
    int posLength = pos;
    pos++;
    pos++;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    

    
    rtcpbuffer[pos++]=(WebRtc_UWord8)0;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0;

    
    int numBoundingSet = 0;
    for(WebRtc_UWord32 n=0; n< boundingSet->lengthOfSet(); n++)
    {
        if (boundingSet->Tmmbr(n) > 0)
        {
            WebRtc_UWord32 tmmbrSSRC = boundingSet->Ssrc(n);
            ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, tmmbrSSRC);
            pos += 4;

            WebRtc_UWord32 bitRate = boundingSet->Tmmbr(n) * 1000;
            WebRtc_UWord32 mmbrExp = 0;
            for(int i=0; i<64; i++)
            {
                if(bitRate <=  ((WebRtc_UWord32)131071 << i))
                {
                    mmbrExp = i;
                    break;
                }
            }
            WebRtc_UWord32 mmbrMantissa = (bitRate >> mmbrExp);
            WebRtc_UWord32 measuredOH = boundingSet->PacketOH(n);

            rtcpbuffer[pos++]=(WebRtc_UWord8)((mmbrExp << 2) + ((mmbrMantissa >> 15) & 0x03));
            rtcpbuffer[pos++]=(WebRtc_UWord8)(mmbrMantissa >> 7);
            rtcpbuffer[pos++]=(WebRtc_UWord8)((mmbrMantissa << 1) + ((measuredOH >> 8)& 0x01));
            rtcpbuffer[pos++]=(WebRtc_UWord8)(measuredOH);
            numBoundingSet++;
        }
    }
    WebRtc_UWord16 length= (WebRtc_UWord16)(2+2*numBoundingSet);
    rtcpbuffer[posLength++]=(WebRtc_UWord8)(length>>8);
    rtcpbuffer[posLength]=(WebRtc_UWord8)(length);
    return 0;
}

WebRtc_Word32
RTCPSender::BuildAPP(WebRtc_UWord8* rtcpbuffer, WebRtc_UWord32& pos)
{
    
    if(_appData == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id, "%s invalid state", __FUNCTION__);
        return -1;
    }
    if(pos + 12 + _appLength >= IP_PACKET_SIZE)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument", __FUNCTION__);
        return -2;
    }
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + _appSubType;

    
    rtcpbuffer[pos++]=(WebRtc_UWord8)204;

    WebRtc_UWord16 length = (_appLength>>2) + 2; 
    rtcpbuffer[pos++]=(WebRtc_UWord8)(length>>8);
    rtcpbuffer[pos++]=(WebRtc_UWord8)(length);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _appName);
    pos += 4;

    
    memcpy(rtcpbuffer +pos, _appData,_appLength);
    pos += _appLength;
    return 0;
}

WebRtc_Word32
RTCPSender::BuildNACK(WebRtc_UWord8* rtcpbuffer,
                      WebRtc_UWord32& pos,
                      const WebRtc_Word32 nackSize,
                      const WebRtc_UWord16* nackList)
{
    
    if(pos + 16 >= IP_PACKET_SIZE)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument", __FUNCTION__);
        return -2;
    }

    
    
    WebRtc_UWord8 FMT = 1;
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + FMT;
    rtcpbuffer[pos++]=(WebRtc_UWord8)205;

    rtcpbuffer[pos++]=(WebRtc_UWord8) 0;
    int nackSizePos = pos;
    rtcpbuffer[pos++]=(WebRtc_UWord8)(3); 

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;

    
    int i = 0;
    int numOfNackFields = 0;
    while(nackSize > i && numOfNackFields < 253)
    {
        WebRtc_UWord16 nack = nackList[i];
        
        ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer+pos, nack);
        pos += 2;

        i++;
        numOfNackFields++;
        if(nackSize > i)
        {
            bool moreThan16Away = (WebRtc_UWord16(nack+16) < nackList[i])?true: false;
            if(!moreThan16Away)
            {
                
                if(WebRtc_UWord16(nack+16) > 0xff00 && nackList[i] < 0x0fff)
                {
                    
                    moreThan16Away = true;
                }
            }
            if(moreThan16Away)
            {
                
                rtcpbuffer[pos++]=(WebRtc_UWord8)0;
                rtcpbuffer[pos++]=(WebRtc_UWord8)0;
            } else
            {
                
                WebRtc_UWord16 bitmask = 0;

                bool within16Away = (WebRtc_UWord16(nack+16) > nackList[i])?true: false;
                if(within16Away)
                {
                   
                    if(WebRtc_UWord16(nack+16) > 0xff00 && nackList[i] < 0x0fff)
                    {
                        
                        within16Away = false;
                    }
                }

                while( nackSize > i && within16Away)
                {
                    WebRtc_Word16 shift = (nackList[i]-nack)-1;
                    assert(!(shift > 15) && !(shift < 0));

                    bitmask += (1<< shift);
                    i++;
                    if(nackSize > i)
                    {
                        within16Away = (WebRtc_UWord16(nack+16) > nackList[i])?true: false;
                        if(within16Away)
                        {
                            
                            if(WebRtc_UWord16(nack+16) > 0xff00 && nackList[i] < 0x0fff)
                            {
                                
                                within16Away = false;
                            }
                        }
                    }
                }
                ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer+pos, bitmask);
                pos += 2;
            }
            
            if(pos + 4 >= IP_PACKET_SIZE)
            {
                return -2;
            }
        } else
        {
            
            rtcpbuffer[pos++]=(WebRtc_UWord8)0;
            rtcpbuffer[pos++]=(WebRtc_UWord8)0;
        }
    }
    rtcpbuffer[nackSizePos]=(WebRtc_UWord8)(2+numOfNackFields);
    return 0;
}

WebRtc_Word32
RTCPSender::BuildBYE(WebRtc_UWord8* rtcpbuffer, WebRtc_UWord32& pos)
{
    
    if(pos + 8 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    if(_includeCSRCs)
    {
        
        rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + 1 + _CSRCs;  
        rtcpbuffer[pos++]=(WebRtc_UWord8)203;

        
        rtcpbuffer[pos++]=(WebRtc_UWord8)0;
        rtcpbuffer[pos++]=(WebRtc_UWord8)(1 + _CSRCs);

        
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
        pos += 4;

        
        for(int i = 0; i < _CSRCs; i++)
        {
            ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _CSRC[i]);
            pos += 4;
        }
    } else
    {
        
        rtcpbuffer[pos++]=(WebRtc_UWord8)0x80 + 1;  
        rtcpbuffer[pos++]=(WebRtc_UWord8)203;

        
        rtcpbuffer[pos++]=(WebRtc_UWord8)0;
        rtcpbuffer[pos++]=(WebRtc_UWord8)1;

        
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
        pos += 4;
    }
    return 0;
}

WebRtc_Word32
RTCPSender::BuildVoIPMetric(WebRtc_UWord8* rtcpbuffer, WebRtc_UWord32& pos)
{
    
    if(pos + 44 >= IP_PACKET_SIZE)
    {
        return -2;
    }

    
    rtcpbuffer[pos++]=(WebRtc_UWord8)0x80;
    rtcpbuffer[pos++]=(WebRtc_UWord8)207;

    WebRtc_UWord32 XRLengthPos = pos;

    
    pos++;
    pos++;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    rtcpbuffer[pos++]=7;
    rtcpbuffer[pos++]=0;
    rtcpbuffer[pos++]=0;
    rtcpbuffer[pos++]=8;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;

    rtcpbuffer[pos++] = _xrVoIPMetric.lossRate;
    rtcpbuffer[pos++] = _xrVoIPMetric.discardRate;
    rtcpbuffer[pos++] = _xrVoIPMetric.burstDensity;
    rtcpbuffer[pos++] = _xrVoIPMetric.gapDensity;

    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.burstDuration >> 8);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.burstDuration);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.gapDuration >> 8);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.gapDuration);

    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.roundTripDelay >> 8);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.roundTripDelay);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.endSystemDelay >> 8);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.endSystemDelay);

    rtcpbuffer[pos++] = _xrVoIPMetric.signalLevel;
    rtcpbuffer[pos++] = _xrVoIPMetric.noiseLevel;
    rtcpbuffer[pos++] = _xrVoIPMetric.RERL;
    rtcpbuffer[pos++] = _xrVoIPMetric.Gmin;

    rtcpbuffer[pos++] = _xrVoIPMetric.Rfactor;
    rtcpbuffer[pos++] = _xrVoIPMetric.extRfactor;
    rtcpbuffer[pos++] = _xrVoIPMetric.MOSLQ;
    rtcpbuffer[pos++] = _xrVoIPMetric.MOSCQ;

    rtcpbuffer[pos++] = _xrVoIPMetric.RXconfig;
    rtcpbuffer[pos++] = 0; 
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.JBnominal >> 8);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.JBnominal);

    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.JBmax >> 8);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.JBmax);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.JBabsMax >> 8);
    rtcpbuffer[pos++] = (WebRtc_UWord8)(_xrVoIPMetric.JBabsMax);

    rtcpbuffer[XRLengthPos]=(WebRtc_UWord8)(0);
    rtcpbuffer[XRLengthPos+1]=(WebRtc_UWord8)(10);
    return 0;
}

WebRtc_Word32
RTCPSender::SendRTCP(const WebRtc_UWord32 packetTypeFlags,
                     const WebRtc_Word32 nackSize,       
                     const WebRtc_UWord16* nackList,     
                     const bool repeat,                  
                     const WebRtc_UWord64 pictureID)     
{
    WebRtc_UWord32 rtcpPacketTypeFlags = packetTypeFlags;
    WebRtc_UWord32 pos = 0;
    WebRtc_UWord8 rtcpbuffer[IP_PACKET_SIZE];

    do  
    {
        
        RTCPReportBlock received;
        bool hasReceived = false;
        WebRtc_UWord32 NTPsec = 0;
        WebRtc_UWord32 NTPfrac = 0;
        bool rtcpCompound = false;
        WebRtc_UWord32 jitterTransmissionOffset = 0;

        {
          CriticalSectionScoped lock(_criticalSectionRTCPSender);
          if(_method == kRtcpOff)
          {
              WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id,
                           "%s invalid state", __FUNCTION__);
              return -1;
          }
          rtcpCompound = (_method == kRtcpCompound) ? true : false;
        }

        if (rtcpCompound ||
            rtcpPacketTypeFlags & kRtcpReport ||
            rtcpPacketTypeFlags & kRtcpSr ||
            rtcpPacketTypeFlags & kRtcpRr)
        {
            
            if(_rtpRtcp.ReportBlockStatistics(&received.fractionLost,
                                              &received.cumulativeLost,
                                              &received.extendedHighSeqNum,
                                              &received.jitter,
                                              &jitterTransmissionOffset) == 0)
            {
                hasReceived = true;

                WebRtc_UWord32 lastReceivedRRNTPsecs = 0;
                WebRtc_UWord32 lastReceivedRRNTPfrac = 0;
                WebRtc_UWord32 remoteSR = 0;

                
                _rtpRtcp.LastReceivedNTP(lastReceivedRRNTPsecs,
                                         lastReceivedRRNTPfrac,
                                         remoteSR);

                
                _clock.CurrentNTP(NTPsec, NTPfrac);

                
                WebRtc_UWord32 delaySinceLastReceivedSR = 0;
                if((lastReceivedRRNTPsecs !=0) || (lastReceivedRRNTPfrac !=0))
                {
                    
                    WebRtc_UWord32 now=NTPsec&0x0000FFFF;
                    now <<=16;
                    now += (NTPfrac&0xffff0000)>>16;

                    WebRtc_UWord32 receiveTime = lastReceivedRRNTPsecs&0x0000FFFF;
                    receiveTime <<=16;
                    receiveTime += (lastReceivedRRNTPfrac&0xffff0000)>>16;

                    delaySinceLastReceivedSR = now-receiveTime;
                }
                received.delaySinceLastSR = delaySinceLastReceivedSR;
                received.lastSR = remoteSR;
            } else
            {
                
                _clock.CurrentNTP(NTPsec, NTPfrac);
            }
        }

        CriticalSectionScoped lock(_criticalSectionRTCPSender);

        if(_TMMBR ) 
        {
            rtcpPacketTypeFlags |= kRtcpTmmbr;
        }
        if(_appSend)
        {
            rtcpPacketTypeFlags |= kRtcpApp;
            _appSend = false;
        }
        if(_REMB && _sendREMB)
        {
            
            
            rtcpPacketTypeFlags |= kRtcpRemb;
        }        
        if(_xrSendVoIPMetric)
        {
            rtcpPacketTypeFlags |= kRtcpXrVoipMetric;
            _xrSendVoIPMetric = false;
        }
        if(_sendTMMBN)  
        {
            rtcpPacketTypeFlags |= kRtcpTmmbn;
            _sendTMMBN = false;
        }

        if(_method == kRtcpCompound)
        {
            if(_sending)
            {
                rtcpPacketTypeFlags |= kRtcpSr;
            } else
            {
                rtcpPacketTypeFlags |= kRtcpRr;
            }
            if (_IJ && hasReceived)
            {
                rtcpPacketTypeFlags |= kRtcpTransmissionTimeOffset;
            }
        } else if(_method == kRtcpNonCompound)
        {
            if(rtcpPacketTypeFlags & kRtcpReport)
            {
                if(_sending)
                {
                    rtcpPacketTypeFlags |= kRtcpSr;
                } else
                {
                    rtcpPacketTypeFlags |= kRtcpRr;
                }
            }
        }
        if( rtcpPacketTypeFlags & kRtcpRr ||
            rtcpPacketTypeFlags & kRtcpSr)
        {
            
            
            WebRtc_Word32 random = rand() % 1000;
            WebRtc_Word32 timeToNext = RTCP_INTERVAL_AUDIO_MS;

            if(_audio)
            {
                timeToNext = (RTCP_INTERVAL_AUDIO_MS/2) + (RTCP_INTERVAL_AUDIO_MS*random/1000);
            }else
            {
                WebRtc_UWord32 minIntervalMs = RTCP_INTERVAL_AUDIO_MS;
                if(_sending)
                {
                    
                    WebRtc_UWord32 sendBitrateKbit = 0;
                    WebRtc_UWord32 videoRate = 0;
                    WebRtc_UWord32 fecRate = 0;
                    WebRtc_UWord32 nackRate = 0;
                    _rtpRtcp.BitrateSent(&sendBitrateKbit,
                                         &videoRate,
                                         &fecRate,
                                         &nackRate);
                    sendBitrateKbit /= 1000;
                    if(sendBitrateKbit != 0)
                    {
                        minIntervalMs = 360000/sendBitrateKbit;
                    }
                }
                if(minIntervalMs > RTCP_INTERVAL_VIDEO_MS)
                {
                    minIntervalMs = RTCP_INTERVAL_VIDEO_MS;
                }
                timeToNext = (minIntervalMs/2) + (minIntervalMs*random/1000);
            }
            _nextTimeToSendRTCP = _clock.GetTimeInMS() + timeToNext;
        }

        
        WebRtc_Word32 buildVal = 0;

        if(rtcpPacketTypeFlags & kRtcpSr)
        {
            if(hasReceived)
            {
                buildVal = BuildSR(rtcpbuffer, pos, NTPsec, NTPfrac, &received);
            } else
            {
                buildVal = BuildSR(rtcpbuffer, pos, NTPsec, NTPfrac);
            }
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
            buildVal = BuildSDEC(rtcpbuffer, pos);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }

        }else if(rtcpPacketTypeFlags & kRtcpRr)
        {
            if(hasReceived)
            {
                buildVal = BuildRR(rtcpbuffer, pos, NTPsec, NTPfrac,&received);
            }else
            {
                buildVal = BuildRR(rtcpbuffer, pos, NTPsec, NTPfrac);
            }
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
            
            if(_CNAME[0] != 0)
            {
                buildVal = BuildSDEC(rtcpbuffer, pos);
                if(buildVal == -1)
                {
                    return -1; 
                }
            }
        }
        if(rtcpPacketTypeFlags & kRtcpTransmissionTimeOffset)
        {
            
            
            buildVal = BuildExtendedJitterReport(rtcpbuffer,
                                                 pos,
                                                 jitterTransmissionOffset);
            if(buildVal == -1)
            {
                return -1; 
            }
            else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpPli)
        {
            buildVal = BuildPLI(rtcpbuffer, pos);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpFir)
        {
            buildVal = BuildFIR(rtcpbuffer, pos, repeat);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpSli)
        {
            buildVal = BuildSLI(rtcpbuffer, pos, (WebRtc_UWord8)pictureID);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpRpsi)
        {
            const WebRtc_Word8 payloadType = _rtpRtcp.SendPayloadType();
            if(payloadType == -1)
            {
                return -1;
            }
            buildVal = BuildRPSI(rtcpbuffer, pos, pictureID, (WebRtc_UWord8)payloadType);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpRemb)
        {
            buildVal = BuildREMB(rtcpbuffer, pos);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpBye)
        {
            buildVal = BuildBYE(rtcpbuffer, pos);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpApp)
        {
            buildVal = BuildAPP(rtcpbuffer, pos);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpTmmbr)
        {
            buildVal = BuildTMMBR(rtcpbuffer, pos);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpTmmbn)
        {
            buildVal = BuildTMMBN(rtcpbuffer, pos);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpNack)
        {
            buildVal = BuildNACK(rtcpbuffer, pos, nackSize, nackList);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
        if(rtcpPacketTypeFlags & kRtcpXrVoipMetric)
        {
            buildVal = BuildVoIPMetric(rtcpbuffer, pos);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
        }
    }while (false);
    
    if (pos == 0)
    {
        return -1;
    }
    return SendToNetwork(rtcpbuffer, (WebRtc_UWord16)pos);
}

WebRtc_Word32
RTCPSender::SendToNetwork(const WebRtc_UWord8* dataBuffer,
                          const WebRtc_UWord16 length)
{
    CriticalSectionScoped lock(_criticalSectionTransport);
    if(_cbTransport)
    {
        if(_cbTransport->SendRTCPPacket(_id, dataBuffer, length) > 0)
        {
            return 0;
        }
    }
    return -1;
}

WebRtc_Word32
RTCPSender::SetCSRCStatus(const bool include)
{
    _includeCSRCs = include;
    return 0;
}

WebRtc_Word32
RTCPSender::SetCSRCs(const WebRtc_UWord32 arrOfCSRC[kRtpCsrcSize],
                    const WebRtc_UWord8 arrLength)
{
    if(arrLength > kRtpCsrcSize)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument", __FUNCTION__);
        assert(false);
        return -1;
    }

    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    for(int i = 0; i < arrLength;i++)
    {
        _CSRC[i] = arrOfCSRC[i];
    }
    _CSRCs = arrLength;
    return 0;
}

WebRtc_Word32
RTCPSender::SetApplicationSpecificData(const WebRtc_UWord8 subType,
                                       const WebRtc_UWord32 name,
                                       const WebRtc_UWord8* data,
                                       const WebRtc_UWord16 length)
{
    if(length %4 != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument", __FUNCTION__);
        return -1;
    }
    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    if(_appData)
    {
        delete [] _appData;
    }

    _appSend = true;
    _appSubType = subType;
    _appName = name;
    _appData = new WebRtc_UWord8[length];
    _appLength = length;
    memcpy(_appData, data, length);
    return 0;
}

WebRtc_Word32
RTCPSender::SetRTCPVoIPMetrics(const RTCPVoIPMetric* VoIPMetric)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    memcpy(&_xrVoIPMetric, VoIPMetric, sizeof(RTCPVoIPMetric));

    _xrSendVoIPMetric = true;
    return 0;
}


WebRtc_Word32 RTCPSender::AddReportBlocks(WebRtc_UWord8* rtcpbuffer,
                                          WebRtc_UWord32& pos,
                                          WebRtc_UWord8& numberOfReportBlocks,
                                          const RTCPReportBlock* received,
                                          const WebRtc_UWord32 NTPsec,
                                          const WebRtc_UWord32 NTPfrac) {
  
  if(pos + 24 >= IP_PACKET_SIZE) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s invalid argument", __FUNCTION__);
    return -1;
  }
  numberOfReportBlocks = _reportBlocks.size();
  if (received) {
    
    numberOfReportBlocks++;
  }
  if (received) {
    
    _lastRTCPTime[0] = ModuleRTPUtility::ConvertNTPTimeToMS(NTPsec, NTPfrac);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;

    
    rtcpbuffer[pos++]=received->fractionLost;

    
    ModuleRTPUtility::AssignUWord24ToBuffer(rtcpbuffer+pos,
                                            received->cumulativeLost);
    pos += 3;
    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos,
                                            received->extendedHighSeqNum);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, received->jitter);
    pos += 4;

    
    
    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, received->lastSR);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos,
                                            received->delaySinceLastSR);
    pos += 4;
  }
  if ((pos + _reportBlocks.size() * 24) >= IP_PACKET_SIZE) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s invalid argument", __FUNCTION__);
    return -1;
  }
  std::map<WebRtc_UWord32, RTCPReportBlock*>::iterator it =
      _reportBlocks.begin();

  for (; it != _reportBlocks.end(); it++) {
    
    WebRtc_UWord32 remoteSSRC = it->first;
    RTCPReportBlock* reportBlock = it->second;
    if (reportBlock) {
      
      ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, remoteSSRC);
      pos += 4;

      
      rtcpbuffer[pos++] = reportBlock->fractionLost;

      
      ModuleRTPUtility::AssignUWord24ToBuffer(rtcpbuffer+pos,
                                              reportBlock->cumulativeLost);
      pos += 3;

      
      ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos,
                                              reportBlock->extendedHighSeqNum);
      pos += 4;

      
      ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos,
                                              reportBlock->jitter);
      pos += 4;

      ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos,
                                              reportBlock->lastSR);
      pos += 4;

      ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos,
                                              reportBlock->delaySinceLastSR);
      pos += 4;
    }
  }
  return pos;
}


WebRtc_Word32
RTCPSender::SetTMMBN(const TMMBRSet* boundingSet,
                     const WebRtc_UWord32 maxBitrateKbit)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    if (0 == _tmmbrHelp.SetTMMBRBoundingSetToSend(boundingSet, maxBitrateKbit))
    {
        _sendTMMBN = true;
        return 0;
    }
    return -1;
}
} 
