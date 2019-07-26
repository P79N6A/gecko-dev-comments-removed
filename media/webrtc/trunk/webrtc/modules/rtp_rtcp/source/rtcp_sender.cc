









#include "rtcp_sender.h"

#include <algorithm>  
#include <cassert>  
#include <cstdlib>  
#include <string.h>  

#include "common_types.h"
#include "modules/rtp_rtcp/source/rtp_rtcp_impl.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/trace.h"
#include "system_wrappers/interface/trace_event.h"

namespace webrtc {

using RTCPUtility::RTCPCnameInformation;

NACKStringBuilder::NACKStringBuilder() :
    _stream(""), _count(0), _consecutive(false)
{
    
}

void NACKStringBuilder::PushNACK(uint16_t nack)
{
    if (_count == 0)
    {
        _stream << nack;
    } else if (nack == _prevNack + 1)
    {
        _consecutive = true;
    } else
    {
        if (_consecutive)
        {
            _stream << "-" << _prevNack;
            _consecutive = false;
        }
        _stream << "," << nack;
    }
    _count++;
    _prevNack = nack;
}

std::string NACKStringBuilder::GetResult()
{
    if (_consecutive)
    {
        _stream << "-" << _prevNack;
        _consecutive = false;
    }
    return _stream.str();
}

RTCPSender::RTCPSender(const int32_t id,
                       const bool audio,
                       Clock* clock,
                       ModuleRtpRtcpImpl* owner) :
    _id(id),
    _audio(audio),
    _clock(clock),
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
    _xrVoIPMetric(),
    _nackCount(0),
    _pliCount(0),
    _fullIntraRequestCount(0)
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
    std::map<uint32_t, RTCPReportBlock*>::iterator it =
        _reportBlocks.begin();
    delete it->second;
    _reportBlocks.erase(it);
  }
  while (!_csrcCNAMEs.empty()) {
    std::map<uint32_t, RTCPCnameInformation*>::iterator it =
        _csrcCNAMEs.begin();
    delete it->second;
    _csrcCNAMEs.erase(it);
  }
  delete _criticalSectionTransport;
  delete _criticalSectionRTCPSender;

  WEBRTC_TRACE(kTraceMemory, kTraceRtpRtcp, _id, "%s deleted", __FUNCTION__);
}

int32_t
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

    _nackCount = 0;
    _pliCount = 0;
    _fullIntraRequestCount = 0;

    return 0;
}

void
RTCPSender::ChangeUniqueId(const int32_t id)
{
    _id = id;
}

int32_t
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

int32_t
RTCPSender::SetRTCPStatus(const RTCPMethod method)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    if(method != kRtcpOff)
    {
        if(_audio)
        {
            _nextTimeToSendRTCP = _clock->TimeInMilliseconds() +
                (RTCP_INTERVAL_AUDIO_MS/2);
        } else
        {
            _nextTimeToSendRTCP = _clock->TimeInMilliseconds() +
                (RTCP_INTERVAL_VIDEO_MS/2);
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

int32_t
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

int32_t
RTCPSender::SetREMBStatus(const bool enable)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _REMB = enable;
    return 0;
}

int32_t
RTCPSender::SetREMBData(const uint32_t bitrate,
                        const uint8_t numberOfSSRC,
                        const uint32_t* SSRC)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _rembBitrate = bitrate;
 
    if(_sizeRembSSRC < numberOfSSRC)
    {
        delete [] _rembSSRC;
        _rembSSRC = new uint32_t[numberOfSSRC];
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

int32_t
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

int32_t
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
  CriticalSectionScoped lock(_criticalSectionRTCPSender);
  last_rtp_timestamp_ = rtp_timestamp;
  if (capture_time_ms < 0) {
    
    last_frame_capture_time_ms_ = _clock->TimeInMilliseconds();
  } else {
    last_frame_capture_time_ms_ = capture_time_ms;
  }
}

void
RTCPSender::SetSSRC( const uint32_t ssrc)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    if(_SSRC != 0)
    {
        
        
        
        _nextTimeToSendRTCP = _clock->TimeInMilliseconds() + 100;
    }
    _SSRC = ssrc;
}

int32_t
RTCPSender::SetRemoteSSRC( const uint32_t ssrc)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _remoteSSRC = ssrc;
    return 0;
}

int32_t
RTCPSender::SetCameraDelay(const int32_t delayMS)
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

int32_t RTCPSender::CNAME(char cName[RTCP_CNAME_SIZE]) {
  assert(cName);
  CriticalSectionScoped lock(_criticalSectionRTCPSender);
  cName[RTCP_CNAME_SIZE - 1] = 0;
  strncpy(cName, _CNAME, RTCP_CNAME_SIZE - 1);
  return 0;
}

int32_t RTCPSender::SetCNAME(const char cName[RTCP_CNAME_SIZE]) {
  if (!cName)
    return -1;

  CriticalSectionScoped lock(_criticalSectionRTCPSender);
  _CNAME[RTCP_CNAME_SIZE - 1] = 0;
  strncpy(_CNAME, cName, RTCP_CNAME_SIZE - 1);
  return 0;
}

int32_t RTCPSender::AddMixedCNAME(const uint32_t SSRC,
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

int32_t RTCPSender::RemoveMixedCNAME(const uint32_t SSRC) {
  CriticalSectionScoped lock(_criticalSectionRTCPSender);
  std::map<uint32_t, RTCPCnameInformation*>::iterator it =
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

























































    int64_t now = _clock->TimeInMilliseconds();

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

uint32_t
RTCPSender::LastSendReport( uint32_t& lastRTCPTime)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);

    lastRTCPTime = _lastRTCPTime[0];
    return _lastSendReport[0];
}

uint32_t
RTCPSender::SendTimeOfSendReport(const uint32_t sendReport)
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

int32_t RTCPSender::AddReportBlock(const uint32_t SSRC,
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
  std::map<uint32_t, RTCPReportBlock*>::iterator it =
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

int32_t RTCPSender::RemoveReportBlock(const uint32_t SSRC) {
  CriticalSectionScoped lock(_criticalSectionRTCPSender);

  std::map<uint32_t, RTCPReportBlock*>::iterator it =
      _reportBlocks.find(SSRC);

  if (it == _reportBlocks.end()) {
    return -1;
  }
  delete it->second;
  _reportBlocks.erase(it);
  return 0;
}

int32_t
RTCPSender::BuildSR(uint8_t* rtcpbuffer,
                    uint32_t& pos,
                    const uint32_t NTPsec,
                    const uint32_t NTPfrac,
                    const RTCPReportBlock* received)
{
    
    if(pos + 52 >= IP_PACKET_SIZE)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument", __FUNCTION__);
        return -2;
    }
    uint32_t RTPtime;

    uint32_t posNumberOfReportBlocks = pos;
    rtcpbuffer[pos++]=(uint8_t)0x80;

    
    rtcpbuffer[pos++]=(uint8_t)200;

    for(int i = (RTCP_NUMBER_OF_SR-2); i >= 0; i--)
    {
        
        _lastSendReport[i+1] = _lastSendReport[i];
        _lastRTCPTime[i+1] =_lastRTCPTime[i];
    }

    _lastRTCPTime[0] = Clock::NtpToMs(NTPsec, NTPfrac);
    _lastSendReport[0] = (NTPsec << 16) + (NTPfrac >> 16);

    uint32_t freqHz = 90000; 
    if(_audio) {
      freqHz =  _rtpRtcp.CurrentSendFrequencyHz();
    }

    
    
    
    
    {
      
      CriticalSectionScoped lock(_criticalSectionRTCPSender);
      RTPtime = start_timestamp_ + last_rtp_timestamp_ + (
          _clock->TimeInMilliseconds() - last_frame_capture_time_ms_) *
          (freqHz / 1000);
    }

    
    
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

    uint8_t numberOfReportBlocks = 0;
    int32_t retVal = AddReportBlocks(rtcpbuffer, pos, numberOfReportBlocks, received, NTPsec, NTPfrac);
    if(retVal < 0)
    {
        
        return retVal ;
    }
    rtcpbuffer[posNumberOfReportBlocks] += numberOfReportBlocks;

    uint16_t len = uint16_t((pos/4) -1);
    ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer+2, len);
    return 0;
}


int32_t RTCPSender::BuildSDEC(uint8_t* rtcpbuffer,
                              uint32_t& pos) {
  size_t lengthCname = strlen(_CNAME);
  assert(lengthCname < RTCP_CNAME_SIZE);

  
  if(pos + 12 + lengthCname  >= IP_PACKET_SIZE) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s invalid argument", __FUNCTION__);
    return -2;
  }
  

  
  rtcpbuffer[pos++] = static_cast<uint8_t>(0x80 + 1 + _csrcCNAMEs.size());
  rtcpbuffer[pos++] = static_cast<uint8_t>(202);

  
  uint32_t SDESLengthPos = pos;
  pos++;
  pos++;

  
  ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
  pos += 4;

  
  rtcpbuffer[pos++] = static_cast<uint8_t>(1);

  
  rtcpbuffer[pos++] = static_cast<uint8_t>(lengthCname);

  uint16_t SDESLength = 10;

  memcpy(&rtcpbuffer[pos], _CNAME, lengthCname);
  pos += lengthCname;
  SDESLength += (uint16_t)lengthCname;

  uint16_t padding = 0;
  
  if ((pos % 4) == 0) {
    padding++;
    rtcpbuffer[pos++]=0;
  }
  while ((pos % 4) != 0) {
    padding++;
    rtcpbuffer[pos++]=0;
  }
  SDESLength += padding;

  std::map<uint32_t, RTCPUtility::RTCPCnameInformation*>::iterator it =
      _csrcCNAMEs.begin();

  for(; it != _csrcCNAMEs.end(); it++) {
    RTCPCnameInformation* cname = it->second;
    uint32_t SSRC = it->first;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, SSRC);
    pos += 4;

    
    rtcpbuffer[pos++] = static_cast<uint8_t>(1);

    size_t length = strlen(cname->name);
    assert(length < RTCP_CNAME_SIZE);

    rtcpbuffer[pos++]= static_cast<uint8_t>(length);
    SDESLength += 6;

    memcpy(&rtcpbuffer[pos],cname->name, length);

    pos += length;
    SDESLength += length;
    uint16_t padding = 0;

    
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
  
  uint16_t buffer_length = (SDESLength / 4) - 1;
  ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer + SDESLengthPos,
                                          buffer_length);
  return 0;
}

int32_t
RTCPSender::BuildRR(uint8_t* rtcpbuffer,
                    uint32_t& pos,
                    const uint32_t NTPsec,
                    const uint32_t NTPfrac,
                    const RTCPReportBlock* received)
{
    
    if(pos + 32 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    uint32_t posNumberOfReportBlocks = pos;

    rtcpbuffer[pos++]=(uint8_t)0x80;
    rtcpbuffer[pos++]=(uint8_t)201;

    
    pos++;
    pos++;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    uint8_t numberOfReportBlocks = 0;
    int32_t retVal = AddReportBlocks(rtcpbuffer, pos, numberOfReportBlocks, received, NTPsec, NTPfrac);
    if(retVal < 0)
    {
        return retVal;
    }
    rtcpbuffer[posNumberOfReportBlocks] += numberOfReportBlocks;

    uint16_t len = uint16_t((pos)/4 -1);
    ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer+2, len);
    return 0;
}



















int32_t
RTCPSender::BuildExtendedJitterReport(
    uint8_t* rtcpbuffer,
    uint32_t& pos,
    const uint32_t jitterTransmissionTimeOffset)
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
    
    uint8_t RC = 1;
    rtcpbuffer[pos++]=(uint8_t)0x80 + RC;
    rtcpbuffer[pos++]=(uint8_t)195;

    
    rtcpbuffer[pos++]=(uint8_t)0;
    rtcpbuffer[pos++]=(uint8_t)(1);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer + pos,
                                            jitterTransmissionTimeOffset);
    pos += 4;
    return 0;
}

int32_t
RTCPSender::BuildPLI(uint8_t* rtcpbuffer, uint32_t& pos)
{
    
    if(pos + 12 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    uint8_t FMT = 1;
    rtcpbuffer[pos++]=(uint8_t)0x80 + FMT;
    rtcpbuffer[pos++]=(uint8_t)206;

    
    rtcpbuffer[pos++]=(uint8_t)0;
    rtcpbuffer[pos++]=(uint8_t)(2);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;
    return 0;
}

int32_t RTCPSender::BuildFIR(uint8_t* rtcpbuffer,
                             uint32_t& pos,
                             bool repeat) {
  
  if(pos + 20 >= IP_PACKET_SIZE)  {
    return -2;
  }
  if (!repeat) {
    _sequenceNumberFIR++;   
  }

  
  uint8_t FMT = 4;
  rtcpbuffer[pos++] = (uint8_t)0x80 + FMT;
  rtcpbuffer[pos++] = (uint8_t)206;

  
  rtcpbuffer[pos++] = (uint8_t)0;
  rtcpbuffer[pos++] = (uint8_t)(4);

  
  ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer + pos, _SSRC);
  pos += 4;

  
  
  rtcpbuffer[pos++] = (uint8_t)0;
  rtcpbuffer[pos++] = (uint8_t)0;
  rtcpbuffer[pos++] = (uint8_t)0;
  rtcpbuffer[pos++] = (uint8_t)0;

  
  ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer + pos, _remoteSSRC);
  pos += 4;

  rtcpbuffer[pos++] = (uint8_t)(_sequenceNumberFIR);
  rtcpbuffer[pos++] = (uint8_t)0;
  rtcpbuffer[pos++] = (uint8_t)0;
  rtcpbuffer[pos++] = (uint8_t)0;
  return 0;
}








int32_t
RTCPSender::BuildSLI(uint8_t* rtcpbuffer, uint32_t& pos, const uint8_t pictureID)
{
    
    if(pos + 16 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    uint8_t FMT = 2;
    rtcpbuffer[pos++]=(uint8_t)0x80 + FMT;
    rtcpbuffer[pos++]=(uint8_t)206;

    
    rtcpbuffer[pos++]=(uint8_t)0;
    rtcpbuffer[pos++]=(uint8_t)(3);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;

    
    
    
    uint32_t sliField = (0x1fff << 6)+ (0x3f & pictureID);
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, sliField);
    pos += 4;
    return 0;
}













int32_t
RTCPSender::BuildRPSI(uint8_t* rtcpbuffer,
                     uint32_t& pos,
                     const uint64_t pictureID,
                     const uint8_t payloadType)
{
    
    if(pos + 24 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    uint8_t FMT = 3;
    rtcpbuffer[pos++]=(uint8_t)0x80 + FMT;
    rtcpbuffer[pos++]=(uint8_t)206;

    
    uint32_t bitsRequired = 7;
    uint8_t bytesRequired = 1;
    while((pictureID>>bitsRequired) > 0)
    {
        bitsRequired += 7;
        bytesRequired++;
    }

    uint8_t size = 3;
    if(bytesRequired > 6)
    {
        size = 5;
    } else if(bytesRequired > 2)
    {
        size = 4;
    }
    rtcpbuffer[pos++]=(uint8_t)0;
    rtcpbuffer[pos++]=size;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;

    
    uint8_t paddingBytes = 4-((2+bytesRequired)%4);
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
        rtcpbuffer[pos] = 0x80 | uint8_t(pictureID >> (i*7));
        pos++;
    }
    
    rtcpbuffer[pos] = uint8_t(pictureID & 0x7f);
    pos++;

    
    for(int j = 0; j <paddingBytes; j++)
    {
        rtcpbuffer[pos] = 0;
        pos++;
    }
    return 0;
}

int32_t
RTCPSender::BuildREMB(uint8_t* rtcpbuffer, uint32_t& pos)
{
    
    if(pos + 20 + 4 * _lengthRembSSRC >= IP_PACKET_SIZE)
    {
        return -2;
    }
    
    uint8_t FMT = 15;
    rtcpbuffer[pos++]=(uint8_t)0x80 + FMT;
    rtcpbuffer[pos++]=(uint8_t)206;

    rtcpbuffer[pos++]=(uint8_t)0;
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
    
    
    uint8_t brExp = 0;
    for(uint32_t i=0; i<64; i++)
    {
        if(_rembBitrate <= ((uint32_t)262143 << i))
        {
            brExp = i;
            break;
        }
    }
    const uint32_t brMantissa = (_rembBitrate >> brExp);
    rtcpbuffer[pos++]=(uint8_t)((brExp << 2) + ((brMantissa >> 16) & 0x03));
    rtcpbuffer[pos++]=(uint8_t)(brMantissa >> 8);
    rtcpbuffer[pos++]=(uint8_t)(brMantissa);

    for (int i = 0; i < _lengthRembSSRC; i++) 
    { 
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _rembSSRC[i]);
        pos += 4;
    }
    TRACE_COUNTER_ID1("webrtc_rtp", "RTCPRembBitrate", _SSRC, _rembBitrate);
    return 0;
}

void
RTCPSender::SetTargetBitrate(unsigned int target_bitrate)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    _tmmbr_Send = target_bitrate / 1000;
}

int32_t
RTCPSender::BuildTMMBR(uint8_t* rtcpbuffer, uint32_t& pos)
{
    
    
    

    
    bool tmmbrOwner = false;
    
    TMMBRSet* candidateSet = _tmmbrHelp.CandidateSet();

    
    
    
    int32_t lengthOfBoundingSet
        = _rtpRtcp.BoundingSet(tmmbrOwner, candidateSet);

    if(lengthOfBoundingSet > 0)
    {
        for (int32_t i = 0; i < lengthOfBoundingSet; i++)
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
        
        uint8_t FMT = 3;
        rtcpbuffer[pos++]=(uint8_t)0x80 + FMT;
        rtcpbuffer[pos++]=(uint8_t)205;

        
        rtcpbuffer[pos++]=(uint8_t)0;
        rtcpbuffer[pos++]=(uint8_t)(4);

        
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
        pos += 4;

        

        
        rtcpbuffer[pos++]=(uint8_t)0;
        rtcpbuffer[pos++]=(uint8_t)0;
        rtcpbuffer[pos++]=(uint8_t)0;
        rtcpbuffer[pos++]=(uint8_t)0;

        
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
        pos += 4;

        uint32_t bitRate = _tmmbr_Send*1000;
        uint32_t mmbrExp = 0;
        for(uint32_t i=0;i<64;i++)
        {
            if(bitRate <= ((uint32_t)131071 << i))
            {
                mmbrExp = i;
                break;
            }
        }
        uint32_t mmbrMantissa = (bitRate >> mmbrExp);

        rtcpbuffer[pos++]=(uint8_t)((mmbrExp << 2) + ((mmbrMantissa >> 15) & 0x03));
        rtcpbuffer[pos++]=(uint8_t)(mmbrMantissa >> 7);
        rtcpbuffer[pos++]=(uint8_t)((mmbrMantissa << 1) + ((_packetOH_Send >> 8)& 0x01));
        rtcpbuffer[pos++]=(uint8_t)(_packetOH_Send);
    }
    return 0;
}

int32_t
RTCPSender::BuildTMMBN(uint8_t* rtcpbuffer, uint32_t& pos)
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
    uint8_t FMT = 4;
    
    rtcpbuffer[pos++]=(uint8_t)0x80 + FMT;
    rtcpbuffer[pos++]=(uint8_t)205;

    
    int posLength = pos;
    pos++;
    pos++;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    

    
    rtcpbuffer[pos++]=(uint8_t)0;
    rtcpbuffer[pos++]=(uint8_t)0;
    rtcpbuffer[pos++]=(uint8_t)0;
    rtcpbuffer[pos++]=(uint8_t)0;

    
    int numBoundingSet = 0;
    for(uint32_t n=0; n< boundingSet->lengthOfSet(); n++)
    {
        if (boundingSet->Tmmbr(n) > 0)
        {
            uint32_t tmmbrSSRC = boundingSet->Ssrc(n);
            ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, tmmbrSSRC);
            pos += 4;

            uint32_t bitRate = boundingSet->Tmmbr(n) * 1000;
            uint32_t mmbrExp = 0;
            for(int i=0; i<64; i++)
            {
                if(bitRate <=  ((uint32_t)131071 << i))
                {
                    mmbrExp = i;
                    break;
                }
            }
            uint32_t mmbrMantissa = (bitRate >> mmbrExp);
            uint32_t measuredOH = boundingSet->PacketOH(n);

            rtcpbuffer[pos++]=(uint8_t)((mmbrExp << 2) + ((mmbrMantissa >> 15) & 0x03));
            rtcpbuffer[pos++]=(uint8_t)(mmbrMantissa >> 7);
            rtcpbuffer[pos++]=(uint8_t)((mmbrMantissa << 1) + ((measuredOH >> 8)& 0x01));
            rtcpbuffer[pos++]=(uint8_t)(measuredOH);
            numBoundingSet++;
        }
    }
    uint16_t length= (uint16_t)(2+2*numBoundingSet);
    rtcpbuffer[posLength++]=(uint8_t)(length>>8);
    rtcpbuffer[posLength]=(uint8_t)(length);
    return 0;
}

int32_t
RTCPSender::BuildAPP(uint8_t* rtcpbuffer, uint32_t& pos)
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
    rtcpbuffer[pos++]=(uint8_t)0x80 + _appSubType;

    
    rtcpbuffer[pos++]=(uint8_t)204;

    uint16_t length = (_appLength>>2) + 2; 
    rtcpbuffer[pos++]=(uint8_t)(length>>8);
    rtcpbuffer[pos++]=(uint8_t)(length);

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _appName);
    pos += 4;

    
    memcpy(rtcpbuffer +pos, _appData,_appLength);
    pos += _appLength;
    return 0;
}

int32_t
RTCPSender::BuildNACK(uint8_t* rtcpbuffer,
                      uint32_t& pos,
                      const int32_t nackSize,
                      const uint16_t* nackList,
                      std::string* nackString)
{
    
    if(pos + 16 >= IP_PACKET_SIZE)
    {
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id, "%s invalid argument", __FUNCTION__);
        return -2;
    }

    
    
    uint8_t FMT = 1;
    rtcpbuffer[pos++]=(uint8_t)0x80 + FMT;
    rtcpbuffer[pos++]=(uint8_t)205;

    rtcpbuffer[pos++]=(uint8_t) 0;
    int nackSizePos = pos;
    rtcpbuffer[pos++]=(uint8_t)(3); 

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
    pos += 4;

    
    ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _remoteSSRC);
    pos += 4;

    NACKStringBuilder stringBuilder;
    
    
    
    int numOfNackFields = 0;
    int maxNackFields = std::min<int>(kRtcpMaxNackFields,
                                      (IP_PACKET_SIZE - pos) / 4);
    int i = 0;
    while (i < nackSize && numOfNackFields < maxNackFields) {
      stringBuilder.PushNACK(nackList[i]);
      uint16_t nack = nackList[i++];
      uint16_t bitmask = 0;
      while (i < nackSize) {
        int shift = static_cast<uint16_t>(nackList[i] - nack) - 1;
        if (shift >= 0 && shift <= 15) {
          stringBuilder.PushNACK(nackList[i]);
          bitmask |= (1 << shift);
          ++i;
        } else {
          break;
        }
      }
      
      assert(pos + 4 < IP_PACKET_SIZE);
      ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer + pos, nack);
      pos += 2;
      ModuleRTPUtility::AssignUWord16ToBuffer(rtcpbuffer + pos, bitmask);
      pos += 2;
      numOfNackFields++;
    }
    if (i != nackSize) {
      WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id,
                   "Nack list to large for one packet.");
    }
    rtcpbuffer[nackSizePos] = static_cast<uint8_t>(2 + numOfNackFields);
    *nackString = stringBuilder.GetResult();
    return 0;
}

int32_t
RTCPSender::BuildBYE(uint8_t* rtcpbuffer, uint32_t& pos)
{
    
    if(pos + 8 >= IP_PACKET_SIZE)
    {
        return -2;
    }
    if(_includeCSRCs)
    {
        
        rtcpbuffer[pos++]=(uint8_t)0x80 + 1 + _CSRCs;  
        rtcpbuffer[pos++]=(uint8_t)203;

        
        rtcpbuffer[pos++]=(uint8_t)0;
        rtcpbuffer[pos++]=(uint8_t)(1 + _CSRCs);

        
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
        pos += 4;

        
        for(int i = 0; i < _CSRCs; i++)
        {
            ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _CSRC[i]);
            pos += 4;
        }
    } else
    {
        
        rtcpbuffer[pos++]=(uint8_t)0x80 + 1;  
        rtcpbuffer[pos++]=(uint8_t)203;

        
        rtcpbuffer[pos++]=(uint8_t)0;
        rtcpbuffer[pos++]=(uint8_t)1;

        
        ModuleRTPUtility::AssignUWord32ToBuffer(rtcpbuffer+pos, _SSRC);
        pos += 4;
    }
    return 0;
}

int32_t
RTCPSender::BuildVoIPMetric(uint8_t* rtcpbuffer, uint32_t& pos)
{
    
    if(pos + 44 >= IP_PACKET_SIZE)
    {
        return -2;
    }

    
    rtcpbuffer[pos++]=(uint8_t)0x80;
    rtcpbuffer[pos++]=(uint8_t)207;

    uint32_t XRLengthPos = pos;

    
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

    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.burstDuration >> 8);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.burstDuration);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.gapDuration >> 8);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.gapDuration);

    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.roundTripDelay >> 8);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.roundTripDelay);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.endSystemDelay >> 8);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.endSystemDelay);

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
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.JBnominal >> 8);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.JBnominal);

    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.JBmax >> 8);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.JBmax);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.JBabsMax >> 8);
    rtcpbuffer[pos++] = (uint8_t)(_xrVoIPMetric.JBabsMax);

    rtcpbuffer[XRLengthPos]=(uint8_t)(0);
    rtcpbuffer[XRLengthPos+1]=(uint8_t)(10);
    return 0;
}

int32_t
RTCPSender::SendRTCP(const uint32_t packetTypeFlags,
                     const int32_t nackSize,       
                     const uint16_t* nackList,     
                     const bool repeat,                  
                     const uint64_t pictureID)     
{
    uint32_t rtcpPacketTypeFlags = packetTypeFlags;
    uint32_t pos = 0;
    uint8_t rtcpbuffer[IP_PACKET_SIZE];

    do  
    {
        
        RTCPReportBlock received;
        bool hasReceived = false;
        uint32_t NTPsec = 0;
        uint32_t NTPfrac = 0;
        bool rtcpCompound = false;
        uint32_t jitterTransmissionOffset = 0;

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

                uint32_t lastReceivedRRNTPsecs = 0;
                uint32_t lastReceivedRRNTPfrac = 0;
                uint32_t remoteSR = 0;

                
                _rtpRtcp.LastReceivedNTP(lastReceivedRRNTPsecs,
                                         lastReceivedRRNTPfrac,
                                         remoteSR);

                
                _clock->CurrentNtp(NTPsec, NTPfrac);

                
                uint32_t delaySinceLastReceivedSR = 0;
                if((lastReceivedRRNTPsecs !=0) || (lastReceivedRRNTPfrac !=0))
                {
                    
                    uint32_t now=NTPsec&0x0000FFFF;
                    now <<=16;
                    now += (NTPfrac&0xffff0000)>>16;

                    uint32_t receiveTime = lastReceivedRRNTPsecs&0x0000FFFF;
                    receiveTime <<=16;
                    receiveTime += (lastReceivedRRNTPfrac&0xffff0000)>>16;

                    delaySinceLastReceivedSR = now-receiveTime;
                }
                received.delaySinceLastSR = delaySinceLastReceivedSR;
                received.lastSR = remoteSR;
            } else
            {
                
                _clock->CurrentNtp(NTPsec, NTPfrac);
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
            
            
            int32_t random = rand() % 1000;
            int32_t timeToNext = RTCP_INTERVAL_AUDIO_MS;

            if(_audio)
            {
                timeToNext = (RTCP_INTERVAL_AUDIO_MS/2) + (RTCP_INTERVAL_AUDIO_MS*random/1000);
            }else
            {
                uint32_t minIntervalMs = RTCP_INTERVAL_AUDIO_MS;
                if(_sending)
                {
                    
                    uint32_t sendBitrateKbit = 0;
                    uint32_t videoRate = 0;
                    uint32_t fecRate = 0;
                    uint32_t nackRate = 0;
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
            _nextTimeToSendRTCP = _clock->TimeInMilliseconds() + timeToNext;
        }

        
        int32_t buildVal = 0;

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
            TRACE_EVENT_INSTANT0("webrtc_rtp", "RTCPSender::PLI");
            _pliCount++;
            TRACE_COUNTER_ID1("webrtc_rtp", "RTCP_PLICount", _SSRC, _pliCount);
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
            TRACE_EVENT_INSTANT0("webrtc_rtp", "RTCPSender::FIR");
            _fullIntraRequestCount++;
            TRACE_COUNTER_ID1("webrtc_rtp", "RTCP_FIRCount", _SSRC,
                              _fullIntraRequestCount);
        }
        if(rtcpPacketTypeFlags & kRtcpSli)
        {
            buildVal = BuildSLI(rtcpbuffer, pos, (uint8_t)pictureID);
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
            const int8_t payloadType = _rtpRtcp.SendPayloadType();
            if(payloadType == -1)
            {
                return -1;
            }
            buildVal = BuildRPSI(rtcpbuffer, pos, pictureID, (uint8_t)payloadType);
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
            TRACE_EVENT_INSTANT0("webrtc_rtp", "RTCPSender::REMB");
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
            std::string nackString;
            buildVal = BuildNACK(rtcpbuffer, pos, nackSize, nackList,
                                 &nackString);
            if(buildVal == -1)
            {
                return -1; 

            }else if(buildVal == -2)
            {
                break;  
            }
            TRACE_EVENT_INSTANT1("webrtc_rtp", "RTCPSender::NACK",
                                 "nacks", TRACE_STR_COPY(nackString.c_str()));
            _nackCount++;
            TRACE_COUNTER_ID1("webrtc_rtp", "RTCP_NACKCount", _SSRC, _nackCount);
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
    return SendToNetwork(rtcpbuffer, (uint16_t)pos);
}

int32_t
RTCPSender::SendToNetwork(const uint8_t* dataBuffer,
                          const uint16_t length)
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

int32_t
RTCPSender::SetCSRCStatus(const bool include)
{
    _includeCSRCs = include;
    return 0;
}

int32_t
RTCPSender::SetCSRCs(const uint32_t arrOfCSRC[kRtpCsrcSize],
                    const uint8_t arrLength)
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

int32_t
RTCPSender::SetApplicationSpecificData(const uint8_t subType,
                                       const uint32_t name,
                                       const uint8_t* data,
                                       const uint16_t length)
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
    _appData = new uint8_t[length];
    _appLength = length;
    memcpy(_appData, data, length);
    return 0;
}

int32_t
RTCPSender::SetRTCPVoIPMetrics(const RTCPVoIPMetric* VoIPMetric)
{
    CriticalSectionScoped lock(_criticalSectionRTCPSender);
    memcpy(&_xrVoIPMetric, VoIPMetric, sizeof(RTCPVoIPMetric));

    _xrSendVoIPMetric = true;
    return 0;
}


int32_t RTCPSender::AddReportBlocks(uint8_t* rtcpbuffer,
                                    uint32_t& pos,
                                    uint8_t& numberOfReportBlocks,
                                    const RTCPReportBlock* received,
                                    const uint32_t NTPsec,
                                    const uint32_t NTPfrac) {
  
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
    
    _lastRTCPTime[0] = Clock::NtpToMs(NTPsec, NTPfrac);

    
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
  std::map<uint32_t, RTCPReportBlock*>::iterator it =
      _reportBlocks.begin();

  for (; it != _reportBlocks.end(); it++) {
    
    uint32_t remoteSSRC = it->first;
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


int32_t
RTCPSender::SetTMMBN(const TMMBRSet* boundingSet,
                     const uint32_t maxBitrateKbit)
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
