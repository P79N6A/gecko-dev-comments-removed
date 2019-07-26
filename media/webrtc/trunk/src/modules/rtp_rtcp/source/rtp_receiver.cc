









#include "trace.h"
#include "rtp_receiver.h"

#include "rtp_rtcp_defines.h"
#include "rtp_rtcp_impl.h"
#include "critical_section_wrapper.h"

#include <cassert>
#include <string.h> 
#include <math.h>   
#include <stdlib.h> 

namespace webrtc {

using ModuleRTPUtility::AudioPayload;
using ModuleRTPUtility::GetCurrentRTP;
using ModuleRTPUtility::Payload;
using ModuleRTPUtility::RTPPayloadParser;
using ModuleRTPUtility::StringCompare;
using ModuleRTPUtility::VideoPayload;

RTPReceiver::RTPReceiver(const WebRtc_Word32 id,
                         const bool audio,
                         RtpRtcpClock* clock,
                         RemoteBitrateEstimator* remote_bitrate,
                         ModuleRtpRtcpImpl* owner) :
    RTPReceiverAudio(id),
    RTPReceiverVideo(id, remote_bitrate, owner),
    Bitrate(clock),
    _id(id),
    _audio(audio),
    _rtpRtcp(*owner),
    _criticalSectionCbs(CriticalSectionWrapper::CreateCriticalSection()),
    _cbRtpFeedback(NULL),
    _cbRtpData(NULL),

    _criticalSectionRTPReceiver(
        CriticalSectionWrapper::CreateCriticalSection()),
    _lastReceiveTime(0),
    _lastReceivedPayloadLength(0),
    _lastReceivedPayloadType(-1),
    _lastReceivedMediaPayloadType(-1),
    _lastReceivedAudioSpecific(),
    _lastReceivedVideoSpecific(),

    _packetTimeOutMS(0),

    _redPayloadType(-1),
    _payloadTypeMap(),
    _rtpHeaderExtensionMap(),
    _SSRC(0),
    _numCSRCs(0),
    _currentRemoteCSRC(),
    _numEnergy(0),
    _currentRemoteEnergy(),
    _useSSRCFilter(false),
    _SSRCFilter(0),

    _jitterQ4(0),
    _jitterMaxQ4(0),
    _cumulativeLoss(0),
    _jitterQ4TransmissionTimeOffset(0),
    _localTimeLastReceivedTimestamp(0),
    _lastReceivedTimestamp(0),
    _lastReceivedSequenceNumber(0),
    _lastReceivedTransmissionTimeOffset(0),

    _receivedSeqFirst(0),
    _receivedSeqMax(0),
    _receivedSeqWraps(0),

    _receivedPacketOH(12), 
    _receivedByteCount(0),
    _receivedOldPacketCount(0),
    _receivedInorderPacketCount(0),

    _lastReportInorderPackets(0),
    _lastReportOldPackets(0),
    _lastReportSeqMax(0),
    _lastReportFractionLost(0),
    _lastReportCumulativeLost(0),
    _lastReportExtendedHighSeqNum(0),
    _lastReportJitter(0),
    _lastReportJitterTransmissionTimeOffset(0),

    _nackMethod(kNackOff),
    _RTX(false),
    _ssrcRTX(0) {
  memset(_currentRemoteCSRC, 0, sizeof(_currentRemoteCSRC));
  memset(_currentRemoteEnergy, 0, sizeof(_currentRemoteEnergy));
  memset(&_lastReceivedAudioSpecific, 0, sizeof(_lastReceivedAudioSpecific));

  _lastReceivedAudioSpecific.channels = 1;
  _lastReceivedVideoSpecific.maxRate = 0;
  _lastReceivedVideoSpecific.videoCodecType = kRtpNoVideo;

  WEBRTC_TRACE(kTraceMemory, kTraceRtpRtcp, id, "%s created", __FUNCTION__);
}

RTPReceiver::~RTPReceiver() {
  if (_cbRtpFeedback) {
    for (int i = 0; i < _numCSRCs; i++) {
      _cbRtpFeedback->OnIncomingCSRCChanged(_id,_currentRemoteCSRC[i], false);
    }
  }
  delete _criticalSectionCbs;
  delete _criticalSectionRTPReceiver;

  while (!_payloadTypeMap.empty()) {
    std::map<WebRtc_Word8, Payload*>::iterator it = _payloadTypeMap.begin();
    delete it->second;
    _payloadTypeMap.erase(it);
  }
  WEBRTC_TRACE(kTraceMemory, kTraceRtpRtcp, _id, "%s deleted", __FUNCTION__);
}

RtpVideoCodecTypes
RTPReceiver::VideoCodecType() const
{
    return _lastReceivedVideoSpecific.videoCodecType;
}

WebRtc_UWord32
RTPReceiver::MaxConfiguredBitrate() const
{
    return _lastReceivedVideoSpecific.maxRate;
}

bool
RTPReceiver::REDPayloadType(const WebRtc_Word8 payloadType) const
{
    return (_redPayloadType == payloadType)?true:false;
}

WebRtc_Word8
RTPReceiver::REDPayloadType() const
{
    return _redPayloadType;
}

    
WebRtc_Word32
RTPReceiver::SetPacketTimeout(const WebRtc_UWord32 timeoutMS)
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    _packetTimeOutMS = timeoutMS;
    return 0;
}

void RTPReceiver::PacketTimeout()
{
    bool packetTimeOut = false;
    {
        CriticalSectionScoped lock(_criticalSectionRTPReceiver);
        if(_packetTimeOutMS == 0)
        {
            
            return;
        }

        if(_lastReceiveTime == 0)
        {
            
            return;
        }

        WebRtc_Word64 now = _clock.GetTimeInMS();

        if(now - _lastReceiveTime > _packetTimeOutMS)
        {
            packetTimeOut = true;
            _lastReceiveTime = 0;  
            _lastReceivedPayloadType = -1; 
            _lastReceivedMediaPayloadType = -1;
        }
    }
    CriticalSectionScoped lock(_criticalSectionCbs);
    if(packetTimeOut && _cbRtpFeedback)
    {
        _cbRtpFeedback->OnPacketTimeout(_id);
    }
}

void
RTPReceiver::ProcessDeadOrAlive(const bool RTCPalive, const WebRtc_Word64 now)
{
    if(_cbRtpFeedback == NULL)
    {
        
        return;
    }
    RTPAliveType alive = kRtpDead;

    if(_lastReceiveTime + 1000 > now)
    {
        
        alive = kRtpAlive;

    } else
    {
        if(RTCPalive)
        {
            if(_audio)
            {
                
                
                if(_lastReceivedPayloadLength < 10) 
                {
                    
                    
                    alive = kRtpNoRtp;
                } else
                {
                    
                }
            } else
            {
                
            }
        }else
        {
            
            
        }
    }


    CriticalSectionScoped lock(_criticalSectionCbs);
    if(_cbRtpFeedback)
    {
        _cbRtpFeedback->OnPeriodicDeadOrAlive(_id, alive);
    }
}

WebRtc_UWord16
RTPReceiver::PacketOHReceived() const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    return _receivedPacketOH;
}

WebRtc_UWord32
RTPReceiver::PacketCountReceived() const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    return _receivedInorderPacketCount;
}

WebRtc_UWord32
RTPReceiver::ByteCountReceived() const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    return _receivedByteCount;
}

WebRtc_Word32
RTPReceiver::RegisterIncomingRTPCallback(RtpFeedback* incomingMessagesCallback)
{
    CriticalSectionScoped lock(_criticalSectionCbs);
    _cbRtpFeedback = incomingMessagesCallback;
    return 0;
}

WebRtc_Word32
RTPReceiver::RegisterIncomingDataCallback(RtpData* incomingDataCallback)
{
    CriticalSectionScoped lock(_criticalSectionCbs);
    _cbRtpData = incomingDataCallback;
    return 0;
}

WebRtc_Word32 RTPReceiver::RegisterReceivePayload(
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const WebRtc_Word8 payloadType,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate) {
  assert(payloadName);
  CriticalSectionScoped lock(_criticalSectionRTPReceiver);

  
  switch (payloadType) {
    
    case 64:        
    case 72:        
    case 73:        
    case 74:        
    case 75:        
    case 76:        
    case 77:        
    case 78:        
    case 79:        
      WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                   "%s invalid payloadtype:%d",
                   __FUNCTION__, payloadType);
      return -1;
    default:
      break;
  }
  size_t payloadNameLength = strlen(payloadName);

  std::map<WebRtc_Word8, Payload*>::iterator it =
      _payloadTypeMap.find(payloadType);
  if (it != _payloadTypeMap.end()) {
    
    Payload* payload = it->second;
    assert(payload);

    size_t nameLength = strlen(payload->name);

    
    
    if (payloadNameLength == nameLength &&
        StringCompare(payload->name, payloadName, payloadNameLength)) {
      if (_audio &&
          payload->audio &&
          payload->typeSpecific.Audio.frequency == frequency &&
          payload->typeSpecific.Audio.channels == channels &&
          (payload->typeSpecific.Audio.rate == rate ||
              payload->typeSpecific.Audio.rate == 0 || rate == 0)) {
        payload->typeSpecific.Audio.rate = rate;
        
        return 0;
      }
      if (!_audio && !payload->audio) {
        
        payload->typeSpecific.Video.maxRate = rate;
        return 0;
      }
    }
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s invalid argument payloadType:%d already registered",
                 __FUNCTION__, payloadType);
    return -1;
  }
  if (_audio) {
    
    
    std::map<WebRtc_Word8, Payload*>::iterator audio_it =
        _payloadTypeMap.begin();
    while (audio_it != _payloadTypeMap.end()) {
      Payload* payload = audio_it->second;
      size_t nameLength = strlen(payload->name);

      if (payloadNameLength == nameLength &&
          StringCompare(payload->name, payloadName, payloadNameLength)) {
        
        
        if (payload->audio) {
          if (payload->typeSpecific.Audio.frequency == frequency &&
              (payload->typeSpecific.Audio.rate == rate ||
                  payload->typeSpecific.Audio.rate == 0 || rate == 0) &&
                  payload->typeSpecific.Audio.channels == channels) {
            
            delete payload;
            _payloadTypeMap.erase(audio_it);
            break;
          }
        } else if(StringCompare(payloadName,"red",3)) {
          delete payload;
          _payloadTypeMap.erase(audio_it);
          break;
        }
      }
      audio_it++;
    }
  }
  Payload* payload = NULL;

  
  
  if (StringCompare(payloadName,"red",3)) {
    _redPayloadType = payloadType;
    payload = new Payload;
    payload->audio = false;
    payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
    strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
  } else {
    if (_audio) {
      payload = RegisterReceiveAudioPayload(payloadName, payloadType,
                                            frequency, channels, rate);
    } else {
      payload = RegisterReceiveVideoPayload(payloadName, payloadType, rate);
    }
  }
  if (payload == NULL) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s filed to register payload",
                 __FUNCTION__);
    return -1;
  }
  _payloadTypeMap[payloadType] = payload;

  
  
  _lastReceivedPayloadType = -1;
  _lastReceivedMediaPayloadType = -1;
  return 0;
}

WebRtc_Word32 RTPReceiver::DeRegisterReceivePayload(
    const WebRtc_Word8 payloadType) {
  CriticalSectionScoped lock(_criticalSectionRTPReceiver);

  std::map<WebRtc_Word8, Payload*>::iterator it =
      _payloadTypeMap.find(payloadType);

  if (it == _payloadTypeMap.end()) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s failed to find payloadType:%d",
                 __FUNCTION__, payloadType);
    return -1;
  }
  delete it->second;
  _payloadTypeMap.erase(it);
  return 0;
}

WebRtc_Word32 RTPReceiver::ReceivePayloadType(
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate,
    WebRtc_Word8* payloadType) const {
  if (payloadType == NULL) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s invalid argument", __FUNCTION__);
    return -1;
  }
  size_t payloadNameLength = strlen(payloadName);

  CriticalSectionScoped lock(_criticalSectionRTPReceiver);

  std::map<WebRtc_Word8, Payload*>::const_iterator it =
      _payloadTypeMap.begin();

  while (it != _payloadTypeMap.end()) {
    Payload* payload = it->second;
    assert(payload);

    size_t nameLength = strlen(payload->name);
    if (payloadNameLength == nameLength &&
        StringCompare(payload->name, payloadName, payloadNameLength)) {
      
      if( payload->audio) {
        if (rate == 0) {
          
          if (payload->typeSpecific.Audio.frequency == frequency &&
              payload->typeSpecific.Audio.channels == channels) {
            *payloadType = it->first;
            return 0;
          }
        } else {
          
          if( payload->typeSpecific.Audio.frequency == frequency &&
              payload->typeSpecific.Audio.channels == channels &&
              payload->typeSpecific.Audio.rate == rate) {
            
            *payloadType = it->first;
            return 0;
          }
        }
      } else {
        
        *payloadType = it->first;
        return 0;
      }
    }
    it++;
  }
  return -1;
}

WebRtc_Word32 RTPReceiver::ReceivePayload(
    const WebRtc_Word8 payloadType,
    char payloadName[RTP_PAYLOAD_NAME_SIZE],
    WebRtc_UWord32* frequency,
    WebRtc_UWord8* channels,
    WebRtc_UWord32* rate) const {
  CriticalSectionScoped lock(_criticalSectionRTPReceiver);

  std::map<WebRtc_Word8, Payload*>::const_iterator it =
      _payloadTypeMap.find(payloadType);

  if (it == _payloadTypeMap.end()) {
    return -1;
  }
  Payload* payload = it->second;
  assert(payload);

  if(frequency) {
    if(payload->audio) {
      *frequency = payload->typeSpecific.Audio.frequency;
    } else {
      *frequency = 90000;
    }
  }
  if (channels) {
    if(payload->audio) {
      *channels = payload->typeSpecific.Audio.channels;
    } else {
      *channels = 1;
    }
  }
  if (rate) {
    if(payload->audio) {
      *rate = payload->typeSpecific.Audio.rate;
    } else {
      assert(false);
      *rate = 0;
    }
  }
  if (payloadName) {
    payloadName[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
    strncpy(payloadName, payload->name, RTP_PAYLOAD_NAME_SIZE - 1);
  }
  return 0;
}

WebRtc_Word32 RTPReceiver::RemotePayload(
    char payloadName[RTP_PAYLOAD_NAME_SIZE],
    WebRtc_Word8* payloadType,
    WebRtc_UWord32* frequency,
    WebRtc_UWord8* channels) const {
  if(_lastReceivedPayloadType == -1) {
    WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id,
                 "%s invalid state", __FUNCTION__);
    return -1;
  }
  std::map<WebRtc_Word8, Payload*>::const_iterator it =
      _payloadTypeMap.find(_lastReceivedPayloadType);

  if (it == _payloadTypeMap.end()) {
    return -1;
  }
  Payload* payload = it->second;
  assert(payload);
  payloadName[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
  strncpy(payloadName, payload->name, RTP_PAYLOAD_NAME_SIZE - 1);

  if (payloadType) {
    *payloadType = _lastReceivedPayloadType;
  }
  if (frequency) {
    if (payload->audio) {
      *frequency = payload->typeSpecific.Audio.frequency;
    } else {
      *frequency = 90000;
    }
  }
  if (channels) {
    if (payload->audio) {
      *channels = payload->typeSpecific.Audio.channels;
    } else {
      *channels = 1;
    }
  }
  return 0;
}

WebRtc_Word32
RTPReceiver::RegisterRtpHeaderExtension(const RTPExtensionType type,
                                        const WebRtc_UWord8 id)
{
    CriticalSectionScoped cs(_criticalSectionRTPReceiver);
    return _rtpHeaderExtensionMap.Register(type, id);
}

WebRtc_Word32
RTPReceiver::DeregisterRtpHeaderExtension(const RTPExtensionType type)
{
    CriticalSectionScoped cs(_criticalSectionRTPReceiver);
    return _rtpHeaderExtensionMap.Deregister(type);
}

void RTPReceiver::GetHeaderExtensionMapCopy(RtpHeaderExtensionMap* map) const
{
    CriticalSectionScoped cs(_criticalSectionRTPReceiver);
    _rtpHeaderExtensionMap.GetCopy(map);
}

NACKMethod
RTPReceiver::NACK() const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    return _nackMethod;
}

    
WebRtc_Word32
RTPReceiver::SetNACKStatus(const NACKMethod method)
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    _nackMethod = method;
    return 0;
}

void RTPReceiver::SetRTXStatus(const bool enable,
                               const WebRtc_UWord32 SSRC) {
  CriticalSectionScoped lock(_criticalSectionRTPReceiver);
  _RTX = enable;
  _ssrcRTX = SSRC;
}

void RTPReceiver::RTXStatus(bool* enable, WebRtc_UWord32* SSRC) const {
  CriticalSectionScoped lock(_criticalSectionRTPReceiver);
  *enable = _RTX;
  *SSRC = _ssrcRTX;
}

WebRtc_UWord32
RTPReceiver::SSRC() const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    return _SSRC;
}

    
WebRtc_Word32
RTPReceiver::CSRCs( WebRtc_UWord32 arrOfCSRC[kRtpCsrcSize]) const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    assert(_numCSRCs <= kRtpCsrcSize);

    if(_numCSRCs >0)
    {
        memcpy(arrOfCSRC, _currentRemoteCSRC, sizeof(WebRtc_UWord32)*_numCSRCs);
    }
    return _numCSRCs;
}

WebRtc_Word32
RTPReceiver::Energy( WebRtc_UWord8 arrOfEnergy[kRtpCsrcSize]) const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    assert(_numEnergy <= kRtpCsrcSize);

    if(_numEnergy >0)
    {
        memcpy(arrOfEnergy, _currentRemoteEnergy, sizeof(WebRtc_UWord8)*_numCSRCs);
    }
    return _numEnergy;
}

WebRtc_Word32 RTPReceiver::IncomingRTPPacket(
    WebRtcRTPHeader* rtp_header,
    const WebRtc_UWord8* packet,
    const WebRtc_UWord16 packet_length) {
  
  
  int length = packet_length - rtp_header->header.paddingLength;

  
  if ((length - rtp_header->header.headerLength) < 0) {
     WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                  "%s invalid argument",
                  __FUNCTION__);
     return -1;
  }
  if (_RTX) {
    if (_ssrcRTX == rtp_header->header.ssrc) {
      
      if (rtp_header->header.headerLength + 2 > packet_length) {
        return -1;
      }
      rtp_header->header.ssrc = _SSRC;
      rtp_header->header.sequenceNumber =
          (packet[rtp_header->header.headerLength] << 8) +
          packet[1 + rtp_header->header.headerLength];
      
      rtp_header->header.headerLength += 2;
    }
  }
  if (_useSSRCFilter) {
    if (rtp_header->header.ssrc != _SSRCFilter) {
      WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id,
                   "%s drop packet due to SSRC filter",
                   __FUNCTION__);
      return -1;
    }
  }
  if (_lastReceiveTime == 0) {
    
    CriticalSectionScoped lock(_criticalSectionCbs);
    if (_cbRtpFeedback) {
      if (length - rtp_header->header.headerLength == 0) {
        
        _cbRtpFeedback->OnReceivedPacket(_id, kPacketKeepAlive);
      } else {
        _cbRtpFeedback->OnReceivedPacket(_id, kPacketRtp);
      }
    }
  }
  WebRtc_Word8 first_payload_byte = 0;
  if (length > 0) {
    first_payload_byte = packet[rtp_header->header.headerLength];
  }
  
  CheckSSRCChanged(rtp_header);

  bool is_red = false;
  VideoPayload video_specific;
  video_specific.maxRate = 0;
  video_specific.videoCodecType = kRtpNoVideo;

  AudioPayload audio_specific;
  audio_specific.channels = 0;
  audio_specific.frequency = 0;

  if (CheckPayloadChanged(rtp_header,
                          first_payload_byte,
                          is_red,
                          audio_specific,
                          video_specific) == -1) {
    if (length - rtp_header->header.headerLength == 0)
    {
      
      WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, _id,
                   "%s received keepalive",
                   __FUNCTION__);
      return 0;
    }
    WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id,
                 "%s received invalid payloadtype",
                 __FUNCTION__);
    return -1;
  }
  CheckCSRC(rtp_header);

  const WebRtc_UWord8* payload_data =
      packet + rtp_header->header.headerLength;

  WebRtc_UWord16 payload_data_length =
      static_cast<WebRtc_UWord16>(length - rtp_header->header.headerLength);

  WebRtc_Word32 retVal = 0;
  if(_audio) {
    retVal = ParseAudioCodecSpecific(rtp_header,
                                     payload_data,
                                     payload_data_length,
                                     audio_specific,
                                     is_red);
  } else {
    retVal = ParseVideoCodecSpecific(rtp_header,
                                     payload_data,
                                     payload_data_length,
                                     video_specific.videoCodecType,
                                     is_red,
                                     packet,
                                     packet_length,
                                     _clock.GetTimeInMS());
  }
  if(retVal < 0) {
    return retVal;
  }

  CriticalSectionScoped lock(_criticalSectionRTPReceiver);

  
  
  bool old_packet = RetransmitOfOldPacket(rtp_header->header.sequenceNumber,
                                          rtp_header->header.timestamp);

  
  UpdateStatistics(rtp_header, payload_data_length, old_packet);

  
  
  _lastReceiveTime = _clock.GetTimeInMS();
  _lastReceivedPayloadLength = payload_data_length;

  if (!old_packet) {
    if (_lastReceivedTimestamp != rtp_header->header.timestamp) {
      _lastReceivedTimestamp = rtp_header->header.timestamp;
    }
    _lastReceivedSequenceNumber = rtp_header->header.sequenceNumber;
    _lastReceivedTransmissionTimeOffset =
        rtp_header->extension.transmissionTimeOffset;
  }
  return retVal;
}


WebRtc_Word32
RTPReceiver::CallbackOfReceivedPayloadData(const WebRtc_UWord8* payloadData,
                                           const WebRtc_UWord16 payloadSize,
                                           const WebRtcRTPHeader* rtpHeader)
{
    CriticalSectionScoped lock(_criticalSectionCbs);
    if(_cbRtpData)
    {
        return _cbRtpData->OnReceivedPayloadData(payloadData, payloadSize, rtpHeader);
    }
    return -1;
}


void
RTPReceiver::UpdateStatistics(const WebRtcRTPHeader* rtpHeader,
                              const WebRtc_UWord16 bytes,
                              const bool oldPacket)
{
    WebRtc_UWord32 freq = 90000;
    if(_audio)
    {
        freq = AudioFrequency();
    }

    Bitrate::Update(bytes);

    _receivedByteCount += bytes;

    if (_receivedSeqMax == 0 && _receivedSeqWraps == 0)
    {
        
        _receivedSeqFirst = rtpHeader->header.sequenceNumber;
        _receivedSeqMax = rtpHeader->header.sequenceNumber;
        _receivedInorderPacketCount = 1;
        _localTimeLastReceivedTimestamp =
            GetCurrentRTP(&_clock, freq); 
        return;
    }

    
    if(InOrderPacket(rtpHeader->header.sequenceNumber))
    {
        const WebRtc_UWord32 RTPtime =
            GetCurrentRTP(&_clock, freq); 
        _receivedInorderPacketCount++;

        
        WebRtc_Word32 seqDiff = rtpHeader->header.sequenceNumber - _receivedSeqMax;
        if (seqDiff < 0)
        {
            
            _receivedSeqWraps++;
        }
        
        _receivedSeqMax = rtpHeader->header.sequenceNumber;

        if (rtpHeader->header.timestamp != _lastReceivedTimestamp &&
            _receivedInorderPacketCount > 1)
        {
            WebRtc_Word32 timeDiffSamples = (RTPtime - _localTimeLastReceivedTimestamp) -
                                          (rtpHeader->header.timestamp - _lastReceivedTimestamp);

            timeDiffSamples = abs(timeDiffSamples);

            
            
            if(timeDiffSamples < 450000)  
            {
                
                WebRtc_Word32 jitterDiffQ4 = (timeDiffSamples << 4) - _jitterQ4;
                _jitterQ4 += ((jitterDiffQ4 + 8) >> 4);
            }

            
            
            WebRtc_Word32 timeDiffSamplesExt =
                (RTPtime - _localTimeLastReceivedTimestamp) -
                ((rtpHeader->header.timestamp +
                  rtpHeader->extension.transmissionTimeOffset) -
                (_lastReceivedTimestamp +
                 _lastReceivedTransmissionTimeOffset));

            timeDiffSamplesExt = abs(timeDiffSamplesExt);

            if(timeDiffSamplesExt < 450000)  
            {
                
                WebRtc_Word32 jitterDiffQ4TransmissionTimeOffset =
                    (timeDiffSamplesExt << 4) - _jitterQ4TransmissionTimeOffset;
                _jitterQ4TransmissionTimeOffset +=
                    ((jitterDiffQ4TransmissionTimeOffset + 8) >> 4);
            }
        }
        _localTimeLastReceivedTimestamp = RTPtime;
    } else
    {
        if(oldPacket)
        {
            _receivedOldPacketCount++;
        }else
        {
            _receivedInorderPacketCount++;
        }
    }

    WebRtc_UWord16 packetOH = rtpHeader->header.headerLength + rtpHeader->header.paddingLength;

    
    
    
    _receivedPacketOH =  (15*_receivedPacketOH + packetOH) >> 4;
}


bool RTPReceiver::RetransmitOfOldPacket(
    const WebRtc_UWord16 sequenceNumber,
    const WebRtc_UWord32 rtpTimeStamp) const {
  if (InOrderPacket(sequenceNumber)) {
    return false;
  }
  WebRtc_UWord32 frequencyKHz = 90;  
  if (_audio) {
    frequencyKHz = AudioFrequency() / 1000;
  }
  WebRtc_Word64 timeDiffMS = _clock.GetTimeInMS() - _lastReceiveTime;
  
  WebRtc_Word32 rtpTimeStampDiffMS = static_cast<WebRtc_Word32>(
      rtpTimeStamp - _lastReceivedTimestamp) / frequencyKHz;

  WebRtc_UWord16 minRTT = 0;
  WebRtc_Word32 maxDelayMs = 0;
  _rtpRtcp.RTT(_SSRC, NULL, NULL, &minRTT, NULL);
  if (minRTT == 0) {
    float jitter = _jitterQ4 >> 4;  
    
    float jitterStd = sqrt(jitter);
    
    
    maxDelayMs = static_cast<WebRtc_Word32>((2 * jitterStd) / frequencyKHz);

    
    if (maxDelayMs == 0) {
      maxDelayMs = 1; 
    }
  } else {
    maxDelayMs = (minRTT / 3) + 1;
  }
  if (timeDiffMS > rtpTimeStampDiffMS + maxDelayMs) {
    return true;
  }
  return false;
}

bool
RTPReceiver::InOrderPacket(const WebRtc_UWord16 sequenceNumber) const
{
    if(_receivedSeqMax >= sequenceNumber)
    {
        if(!(_receivedSeqMax > 0xff00 && sequenceNumber < 0x0ff ))
        {
            if(_receivedSeqMax - NACK_PACKETS_MAX_SIZE > sequenceNumber)
            {
                
            }else
            {
                
                return false;
            }
        }
    }else
    {
        
        if(sequenceNumber > 0xff00 && _receivedSeqMax < 0x0ff )
        {
            if(_receivedSeqMax - NACK_PACKETS_MAX_SIZE > sequenceNumber)
            {
                
            }else
            {
                
                return false;
            }
        }
    }
    return true;
}

WebRtc_UWord16
RTPReceiver::SequenceNumber() const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    return _lastReceivedSequenceNumber;
}

WebRtc_UWord32
RTPReceiver::TimeStamp() const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    return _lastReceivedTimestamp;
}

WebRtc_UWord32 RTPReceiver::PayloadTypeToPayload(
    const WebRtc_UWord8 payloadType,
    Payload*& payload) const {

  std::map<WebRtc_Word8, Payload*>::const_iterator it =
      _payloadTypeMap.find(payloadType);

  
  if (it == _payloadTypeMap.end()) {
    return -1;
  }
  payload = it->second;
  return 0;
}


WebRtc_Word32
RTPReceiver::EstimatedRemoteTimeStamp(WebRtc_UWord32& timestamp) const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    WebRtc_UWord32 freq = 90000;
    if(_audio)
    {
        freq = AudioFrequency();
    }
    if(_localTimeLastReceivedTimestamp == 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id, "%s invalid state", __FUNCTION__);
        return -1;
    }
    
    WebRtc_UWord32 diff = GetCurrentRTP(&_clock, freq)
        - _localTimeLastReceivedTimestamp;

    timestamp = _lastReceivedTimestamp + diff;
    return 0;
}

    
WebRtc_Word32
RTPReceiver::SSRCFilter(WebRtc_UWord32& allowedSSRC) const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);
    if(_useSSRCFilter)
    {
        allowedSSRC = _SSRCFilter;
        return 0;
    }
    WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id, "%s invalid state", __FUNCTION__);
    return -1;
}

    
WebRtc_Word32
RTPReceiver::SetSSRCFilter(const bool enable, const WebRtc_UWord32 allowedSSRC)
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    _useSSRCFilter = enable;
    if(enable)
    {
        _SSRCFilter = allowedSSRC;
    } else
    {
        _SSRCFilter = 0;
    }
    return 0;
}


void RTPReceiver::CheckSSRCChanged(const WebRtcRTPHeader* rtpHeader) {
  bool newSSRC = false;
  bool reInitializeDecoder = false;
  char payloadName[RTP_PAYLOAD_NAME_SIZE];
  WebRtc_UWord32 frequency = 90000; 
  WebRtc_UWord8 channels = 1;
  WebRtc_UWord32 rate = 0;

  {
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    if (_SSRC != rtpHeader->header.ssrc ||
        (_lastReceivedPayloadType == -1 && _SSRC == 0)) {
      
      newSSRC = true;

      
      ResetStatistics();

      _lastReceivedTimestamp      = 0;
      _lastReceivedSequenceNumber = 0;
      _lastReceivedTransmissionTimeOffset = 0;

      if (_SSRC) {  
        
        if (rtpHeader->header.payloadType == _lastReceivedPayloadType) {
          reInitializeDecoder = true;

          std::map<WebRtc_Word8, Payload*>::iterator it =
              _payloadTypeMap.find(rtpHeader->header.payloadType);

          if (it == _payloadTypeMap.end()) {
            return;
          }
          Payload* payload = it->second;
          assert(payload);
          payloadName[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
          strncpy(payloadName, payload->name, RTP_PAYLOAD_NAME_SIZE - 1);
          if(payload->audio) {
            frequency = payload->typeSpecific.Audio.frequency;
            channels =  payload->typeSpecific.Audio.channels;
            rate = payload->typeSpecific.Audio.rate;
          } else {
            frequency = 90000;
          }
        }
      }
      _SSRC = rtpHeader->header.ssrc;
    }
  }
  if(newSSRC) {
    
    
    _rtpRtcp.SetRemoteSSRC(rtpHeader->header.ssrc);
  }
  CriticalSectionScoped lock(_criticalSectionCbs);
  if(_cbRtpFeedback) {
    if(newSSRC) {
      _cbRtpFeedback->OnIncomingSSRCChanged(_id, rtpHeader->header.ssrc);
    }
    if(reInitializeDecoder) {
      if (-1 == _cbRtpFeedback->OnInitializeDecoder(_id,
          rtpHeader->header.payloadType, payloadName, frequency, channels,
          rate)) {  
        WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                     "Failed to create decoder for payload type:%d",
                     rtpHeader->header.payloadType);
      }
    }
  }
}


WebRtc_Word32 RTPReceiver::CheckPayloadChanged(
    const WebRtcRTPHeader* rtpHeader,
    const WebRtc_Word8 firstPayloadByte,
    bool& isRED,
    AudioPayload& audioSpecificPayload,
    VideoPayload& videoSpecificPayload) {
  bool reInitializeDecoder = false;

  char payloadName[RTP_PAYLOAD_NAME_SIZE];
  WebRtc_Word8 payloadType = rtpHeader->header.payloadType;

  {
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    if (payloadType != _lastReceivedPayloadType) {
      if (REDPayloadType(payloadType)) {
        
        payloadType = firstPayloadByte & 0x7f;
        isRED = true;

        if (REDPayloadType(payloadType)) {
            
            
            
            return -1;
        }

        
        if (payloadType == _lastReceivedPayloadType) {
          if(_audio)
          {
            memcpy(&audioSpecificPayload, &_lastReceivedAudioSpecific,
                   sizeof(_lastReceivedAudioSpecific));
          } else {
            memcpy(&videoSpecificPayload, &_lastReceivedVideoSpecific,
                   sizeof(_lastReceivedVideoSpecific));
          }
          return 0;
        }
      }
      if (_audio) {
        if (TelephoneEventPayloadType(payloadType)) {
          
          isRED = false;
          return 0;
        }
        
        if (CNGPayloadType(payloadType, audioSpecificPayload.frequency)) {
          
          isRED = false;
          return 0;
        }
      }
      std::map<WebRtc_Word8, ModuleRTPUtility::Payload*>::iterator it =
          _payloadTypeMap.find(payloadType);

      
      if (it == _payloadTypeMap.end()) {
        return -1;
      }
      Payload* payload = it->second;
      assert(payload);
      payloadName[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
      strncpy(payloadName, payload->name, RTP_PAYLOAD_NAME_SIZE - 1);
      _lastReceivedPayloadType = payloadType;

      reInitializeDecoder = true;

      if(payload->audio) {
        memcpy(&_lastReceivedAudioSpecific, &(payload->typeSpecific.Audio),
               sizeof(_lastReceivedAudioSpecific));
        memcpy(&audioSpecificPayload, &(payload->typeSpecific.Audio),
               sizeof(_lastReceivedAudioSpecific));
      } else {
        memcpy(&_lastReceivedVideoSpecific, &(payload->typeSpecific.Video),
               sizeof(_lastReceivedVideoSpecific));
        memcpy(&videoSpecificPayload, &(payload->typeSpecific.Video),
               sizeof(_lastReceivedVideoSpecific));

        if (_lastReceivedVideoSpecific.videoCodecType == kRtpFecVideo)
        {
          
          reInitializeDecoder = false;
        } else {
          if (_lastReceivedMediaPayloadType == _lastReceivedPayloadType) {
            
            reInitializeDecoder = false;
          }
          _lastReceivedMediaPayloadType = _lastReceivedPayloadType;
        }
      }
      if (reInitializeDecoder) {
        
        ResetStatistics();
      }
    } else {
      if(_audio)
      {
        memcpy(&audioSpecificPayload, &_lastReceivedAudioSpecific,
               sizeof(_lastReceivedAudioSpecific));
      } else
      {
        memcpy(&videoSpecificPayload, &_lastReceivedVideoSpecific,
               sizeof(_lastReceivedVideoSpecific));
      }
      isRED = false;
    }
  }   
  if (reInitializeDecoder) {
    CriticalSectionScoped lock(_criticalSectionCbs);
    if (_cbRtpFeedback) {
      
      if(_audio) {
        if (-1 == _cbRtpFeedback->OnInitializeDecoder(_id, payloadType,
            payloadName, audioSpecificPayload.frequency,
            audioSpecificPayload.channels, audioSpecificPayload.rate)) {
          WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                       "Failed to create audio decoder for payload type:%d",
                       payloadType);
          return -1; 
        }
      } else {
        if (-1 == _cbRtpFeedback->OnInitializeDecoder(_id, payloadType,
            payloadName, 90000, 1, 0)) {
          WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                       "Failed to create video decoder for payload type:%d",
                       payloadType);
          return -1; 
        }
      }
    }
  }
  return 0;
}


void RTPReceiver::CheckCSRC(const WebRtcRTPHeader* rtpHeader) {
  WebRtc_Word32 numCSRCsDiff = 0;
  WebRtc_UWord32 oldRemoteCSRC[kRtpCsrcSize];
  WebRtc_UWord8 oldNumCSRCs = 0;

  {
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    if (TelephoneEventPayloadType(rtpHeader->header.payloadType)) {
      
      return;
    }
    _numEnergy = rtpHeader->type.Audio.numEnergy;
    if (rtpHeader->type.Audio.numEnergy > 0 &&
        rtpHeader->type.Audio.numEnergy <= kRtpCsrcSize) {
      memcpy(_currentRemoteEnergy,
             rtpHeader->type.Audio.arrOfEnergy,
             rtpHeader->type.Audio.numEnergy);
    }
    oldNumCSRCs  = _numCSRCs;
    if (oldNumCSRCs > 0) {
      
      memcpy(oldRemoteCSRC, _currentRemoteCSRC,
             _numCSRCs * sizeof(WebRtc_UWord32));
    }
    const WebRtc_UWord8 numCSRCs = rtpHeader->header.numCSRCs;
    if ((numCSRCs > 0) && (numCSRCs <= kRtpCsrcSize)) {
      
      memcpy(_currentRemoteCSRC,
             rtpHeader->header.arrOfCSRCs,
             numCSRCs * sizeof(WebRtc_UWord32));
    }
    if (numCSRCs > 0 || oldNumCSRCs > 0) {
      numCSRCsDiff = numCSRCs - oldNumCSRCs;
      _numCSRCs = numCSRCs;  
    } else {
      
      return;
    }
  }  

  CriticalSectionScoped lock(_criticalSectionCbs);
  if (_cbRtpFeedback == NULL) {
    return;
  }
  bool haveCalledCallback = false;
  
  for (WebRtc_UWord8 i = 0; i < rtpHeader->header.numCSRCs; ++i) {
    const WebRtc_UWord32 csrc = rtpHeader->header.arrOfCSRCs[i];

    bool foundMatch = false;
    for (WebRtc_UWord8 j = 0; j < oldNumCSRCs; ++j) {
      if (csrc == oldRemoteCSRC[j]) {  
        foundMatch = true;
        break;
      }
    }
    if (!foundMatch && csrc) {
      
      haveCalledCallback = true;
      _cbRtpFeedback->OnIncomingCSRCChanged(_id, csrc, true);
    }
  }
  
  for (WebRtc_UWord8 i = 0; i < oldNumCSRCs; ++i) {
    const WebRtc_UWord32 csrc = oldRemoteCSRC[i];

    bool foundMatch = false;
    for (WebRtc_UWord8 j = 0; j < rtpHeader->header.numCSRCs; ++j) {
      if (csrc == rtpHeader->header.arrOfCSRCs[j]) {
        foundMatch = true;
        break;
      }
    }
    if (!foundMatch && csrc) {
      
      haveCalledCallback = true;
      _cbRtpFeedback->OnIncomingCSRCChanged(_id, csrc, false);
    }
  }
  if (!haveCalledCallback) {
    
    
    
    if (numCSRCsDiff > 0) {
      _cbRtpFeedback->OnIncomingCSRCChanged(_id, 0, true);
    } else if (numCSRCsDiff < 0) {
      _cbRtpFeedback->OnIncomingCSRCChanged(_id, 0, false);
    }
  }
}

WebRtc_Word32
RTPReceiver::ResetStatistics()
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    _lastReportInorderPackets = 0;
    _lastReportOldPackets = 0;
    _lastReportSeqMax = 0;
    _lastReportFractionLost = 0;
    _lastReportCumulativeLost = 0;
    _lastReportExtendedHighSeqNum = 0;
    _lastReportJitter = 0;
    _lastReportJitterTransmissionTimeOffset = 0;
    _jitterQ4 = 0;
    _jitterMaxQ4 = 0;
    _cumulativeLoss = 0;
    _jitterQ4TransmissionTimeOffset = 0;
    _receivedSeqWraps = 0;
    _receivedSeqMax = 0;
    _receivedSeqFirst = 0;
    _receivedByteCount = 0;
    _receivedOldPacketCount = 0;
    _receivedInorderPacketCount = 0;
    return 0;
}

WebRtc_Word32
RTPReceiver::ResetDataCounters()
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    _receivedByteCount = 0;
    _receivedOldPacketCount = 0;
    _receivedInorderPacketCount = 0;
    _lastReportInorderPackets = 0;

    return 0;
}

WebRtc_Word32
RTPReceiver::Statistics(WebRtc_UWord8  *fraction_lost,
                       WebRtc_UWord32 *cum_lost,
                       WebRtc_UWord32 *ext_max,
                       WebRtc_UWord32 *jitter,
                       WebRtc_UWord32 *max_jitter,
                       WebRtc_UWord32 *jitter_transmission_time_offset,
                       bool reset) const
{
    WebRtc_Word32 missing;
    return Statistics(fraction_lost,
                      cum_lost,
                      ext_max,
                      jitter,
                      max_jitter,
                      jitter_transmission_time_offset,
                      &missing,
                      reset);
}

WebRtc_Word32
RTPReceiver::Statistics(WebRtc_UWord8  *fraction_lost,
                        WebRtc_UWord32 *cum_lost,
                        WebRtc_UWord32 *ext_max,
                        WebRtc_UWord32 *jitter,
                        WebRtc_UWord32 *max_jitter,
                        WebRtc_UWord32 *jitter_transmission_time_offset,
                        WebRtc_Word32  *missing,
                        bool reset) const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    if (missing == NULL)
    {
        return -1;
    }
    if(_receivedSeqFirst == 0 && _receivedByteCount == 0)
    {
        
        
        return -1;
    }
    if(!reset)
    {
        if(_lastReportInorderPackets == 0)
        {
            
            return -1;
        }
        
        if(fraction_lost)
        {
            *fraction_lost = _lastReportFractionLost;
        }
        if(cum_lost)
        {
            *cum_lost = _lastReportCumulativeLost;  
        }
        if(ext_max)
        {
            *ext_max = _lastReportExtendedHighSeqNum;
        }
        if(jitter)
        {
            *jitter =_lastReportJitter;
        }
        if(max_jitter)
        {
            
            
            *max_jitter = (_jitterMaxQ4 >> 4);
        }
        if(jitter_transmission_time_offset)
        {
            *jitter_transmission_time_offset =
               _lastReportJitterTransmissionTimeOffset;
        }
        return 0;
    }

    if (_lastReportInorderPackets == 0)
    {
        
        _lastReportSeqMax = _receivedSeqFirst-1;
    }
    


    WebRtc_UWord16 expSinceLast = (_receivedSeqMax - _lastReportSeqMax);

    if(_lastReportSeqMax > _receivedSeqMax)
    {
        
        expSinceLast = 0;
    }

    
    WebRtc_UWord32 recSinceLast = _receivedInorderPacketCount - _lastReportInorderPackets;

    if(_nackMethod == kNackOff)
    {
        
        WebRtc_UWord32 oldPackets = _receivedOldPacketCount - _lastReportOldPackets;
        recSinceLast += oldPackets;
    }else
    {
        
        
        
        
        

        
        
    }

    *missing = 0;
    if(expSinceLast > recSinceLast)
    {
        *missing = (expSinceLast - recSinceLast);
    }
    WebRtc_UWord8 fractionLost = 0;
    if(expSinceLast)
    {
        
        fractionLost = (WebRtc_UWord8) ((255 * (*missing)) / expSinceLast);
    }
    if(fraction_lost)
    {
        *fraction_lost = fractionLost;
    }
    
    _cumulativeLoss += *missing;

    if(_jitterQ4 > _jitterMaxQ4)
    {
        _jitterMaxQ4 = _jitterQ4;
    }
    if(cum_lost)
    {
        *cum_lost =  _cumulativeLoss;
    }
    if(ext_max)
    {
        *ext_max = (_receivedSeqWraps<<16) + _receivedSeqMax;
    }
    if(jitter)
    {
        
        
        *jitter = (_jitterQ4 >> 4);
    }
    if(max_jitter)
    {
        
        
        *max_jitter = (_jitterMaxQ4 >> 4);
    }
    if(jitter_transmission_time_offset)
    {
        
        
        *jitter_transmission_time_offset =
            (_jitterQ4TransmissionTimeOffset >> 4);
    }
    if(reset)
    {
        
        _lastReportFractionLost = fractionLost;
        _lastReportCumulativeLost = _cumulativeLoss;  
        _lastReportExtendedHighSeqNum = (_receivedSeqWraps<<16) + _receivedSeqMax;
        _lastReportJitter  = (_jitterQ4 >> 4);
        _lastReportJitterTransmissionTimeOffset =
            (_jitterQ4TransmissionTimeOffset >> 4);

        
        _lastReportInorderPackets = _receivedInorderPacketCount;
        _lastReportOldPackets = _receivedOldPacketCount;
        _lastReportSeqMax = _receivedSeqMax;
    }
    return 0;
}

WebRtc_Word32
RTPReceiver::DataCounters(WebRtc_UWord32 *bytesReceived,
                          WebRtc_UWord32 *packetsReceived) const
{
    CriticalSectionScoped lock(_criticalSectionRTPReceiver);

    if(bytesReceived)
    {
        *bytesReceived = _receivedByteCount;
    }
    if(packetsReceived)
    {
        *packetsReceived = _receivedOldPacketCount + _receivedInorderPacketCount;
    }
    return 0;
}

void
RTPReceiver::ProcessBitrate()
{
    CriticalSectionScoped cs(_criticalSectionRTPReceiver);

    Bitrate::Process();
}
} 
