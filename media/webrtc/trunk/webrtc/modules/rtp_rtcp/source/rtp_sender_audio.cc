









#include "rtp_sender_audio.h"

#include <string.h> 
#include <cassert> 

namespace webrtc {
RTPSenderAudio::RTPSenderAudio(const WebRtc_Word32 id, RtpRtcpClock* clock,
                               RTPSenderInterface* rtpSender) :
    _id(id),
    _clock(*clock),
    _rtpSender(rtpSender),
    _audioFeedbackCritsect(CriticalSectionWrapper::CreateCriticalSection()),
    _audioFeedback(NULL),
    _sendAudioCritsect(CriticalSectionWrapper::CreateCriticalSection()),
    _frequency(8000),
    _packetSizeSamples(160),
    _dtmfEventIsOn(false),
    _dtmfEventFirstPacketSent(false),
    _dtmfPayloadType(-1),
    _dtmfTimestamp(0),
    _dtmfKey(0),
    _dtmfLengthSamples(0),
    _dtmfLevel(0),
    _dtmfTimeLastSent(0),
    _dtmfTimestampLastSent(0),
    _REDPayloadType(-1),
    _inbandVADactive(false),
    _cngNBPayloadType(-1),
    _cngWBPayloadType(-1),
    _cngSWBPayloadType(-1),
    _cngFBPayloadType(-1),
    _lastPayloadType(-1),
    _includeAudioLevelIndication(false),    
    _audioLevelIndicationID(0),
    _audioLevel_dBov(0) {
};

RTPSenderAudio::~RTPSenderAudio()
{
    delete _sendAudioCritsect;
    delete _audioFeedbackCritsect;
}

WebRtc_Word32
RTPSenderAudio::RegisterAudioCallback(RtpAudioFeedback* messagesCallback)
{
    CriticalSectionScoped cs(_audioFeedbackCritsect);
    _audioFeedback = messagesCallback;
    return 0;
}

void
RTPSenderAudio::SetAudioFrequency(const WebRtc_UWord32 f)
{
    CriticalSectionScoped cs(_sendAudioCritsect);
    _frequency = f;
}

int
RTPSenderAudio::AudioFrequency() const
{
    CriticalSectionScoped cs(_sendAudioCritsect);
    return _frequency;
}

    
WebRtc_Word32
RTPSenderAudio::SetAudioPacketSize(const WebRtc_UWord16 packetSizeSamples)
{
    CriticalSectionScoped cs(_sendAudioCritsect);

    _packetSizeSamples = packetSizeSamples;
    return 0;
}

WebRtc_Word32 RTPSenderAudio::RegisterAudioPayload(
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const WebRtc_Word8 payloadType,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate,
    ModuleRTPUtility::Payload*& payload) {
  CriticalSectionScoped cs(_sendAudioCritsect);

  if (ModuleRTPUtility::StringCompare(payloadName, "cn", 2))  {
    
    if (frequency == 8000) {
      _cngNBPayloadType = payloadType;

    } else if (frequency == 16000) {
      _cngWBPayloadType = payloadType;

    } else if (frequency == 32000) {
      _cngSWBPayloadType = payloadType;

    } else if (frequency == 48000) {
      _cngFBPayloadType = payloadType;

    } else {
      return -1;
    }
  }
  if (ModuleRTPUtility::StringCompare(payloadName, "telephone-event", 15)) {
    
    
    _dtmfPayloadType = payloadType;
    return 0;
    
  }
  payload = new ModuleRTPUtility::Payload;
  payload->typeSpecific.Audio.frequency = frequency;
  payload->typeSpecific.Audio.channels = channels;
  payload->typeSpecific.Audio.rate = rate;
  payload->audio = true;
  payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
  strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
  return 0;
}

bool
RTPSenderAudio::MarkerBit(const FrameType frameType,
                          const WebRtc_Word8 payloadType)
{
    CriticalSectionScoped cs(_sendAudioCritsect);

    
    bool markerBit = false;
    if(_lastPayloadType != payloadType)
    {
        if(_cngNBPayloadType != -1)
        {
            
            if(_cngNBPayloadType == payloadType)
            {
                
                return false;
            }
        }
        if(_cngWBPayloadType != -1)
        {
            
            if(_cngWBPayloadType == payloadType)
            {
                
                return false;
            }
        }
        if(_cngSWBPayloadType != -1)
        {
            
            if(_cngSWBPayloadType == payloadType)
            {
                
                return false;
            }
        }
        if(_cngFBPayloadType != -1)
        {
            
            if(_cngFBPayloadType == payloadType)
            {
                
                return false;
            }
        }
        
        if(_lastPayloadType == -1)
        {
            if(frameType != kAudioFrameCN)
            {
                
                return true;

            }else
            {
                
                _inbandVADactive = true;
                return false;
            }
        }
        
        
        

        
        markerBit = true;
    }

    
    if(frameType == kAudioFrameCN)
    {
        _inbandVADactive = true;

    } else if(_inbandVADactive)
    {
        _inbandVADactive = false;
        markerBit = true;
    }
    return markerBit;
}

bool
RTPSenderAudio::SendTelephoneEventActive(WebRtc_Word8& telephoneEvent) const
{
    if(_dtmfEventIsOn)
    {
        telephoneEvent = _dtmfKey;
        return true;
    }
    WebRtc_Word64 delaySinceLastDTMF = _clock.GetTimeInMS() - _dtmfTimeLastSent;
    if(delaySinceLastDTMF < 100)
    {
        telephoneEvent = _dtmfKey;
        return true;
    }
    telephoneEvent = -1;
    return false;
}

WebRtc_Word32 RTPSenderAudio::SendAudio(
    const FrameType frameType,
    const WebRtc_Word8 payloadType,
    const WebRtc_UWord32 captureTimeStamp,
    const WebRtc_UWord8* payloadData,
    const WebRtc_UWord32 dataSize,
    const RTPFragmentationHeader* fragmentation) {
  
  WebRtc_UWord16 payloadSize = static_cast<WebRtc_UWord16>(dataSize);
  WebRtc_UWord16 maxPayloadLength = _rtpSender->MaxPayloadLength();
  bool dtmfToneStarted = false;
  WebRtc_UWord16 dtmfLengthMS = 0;
  WebRtc_UWord8 key = 0;

  
  if (!_dtmfEventIsOn && PendingDTMF()) {
    CriticalSectionScoped cs(_sendAudioCritsect);

    WebRtc_Word64 delaySinceLastDTMF = _clock.GetTimeInMS() - _dtmfTimeLastSent;

    if (delaySinceLastDTMF > 100) {
      
      _dtmfTimestamp = captureTimeStamp;
      if (NextDTMF(&key, &dtmfLengthMS, &_dtmfLevel) >= 0) {
        _dtmfEventFirstPacketSent = false;
        _dtmfKey = key;
        _dtmfLengthSamples = (_frequency / 1000) * dtmfLengthMS;
        dtmfToneStarted = true;
        _dtmfEventIsOn = true;
      }
    }
  }
  if (dtmfToneStarted) {
    CriticalSectionScoped cs(_audioFeedbackCritsect);
    if (_audioFeedback) {
      _audioFeedback->OnPlayTelephoneEvent(_id, key, dtmfLengthMS, _dtmfLevel);
    }
  }

  
  
  {
    _sendAudioCritsect->Enter();

    if (_dtmfEventIsOn) {
      if (frameType == kFrameEmpty) {
        
        
        
        if (_packetSizeSamples > (captureTimeStamp - _dtmfTimestampLastSent)) {
          
          _sendAudioCritsect->Leave();
          return 0;
        }
      }
      _dtmfTimestampLastSent = captureTimeStamp;
      WebRtc_UWord32 dtmfDurationSamples = captureTimeStamp - _dtmfTimestamp;
      bool ended = false;
      bool send = true;

      if (_dtmfLengthSamples > dtmfDurationSamples) {
        if (dtmfDurationSamples <= 0) {
          
          send = false;
        }
      } else {
        ended = true;
        _dtmfEventIsOn = false;
        _dtmfTimeLastSent = _clock.GetTimeInMS();
      }
      
      _sendAudioCritsect->Leave();
      if (send) {
        if (dtmfDurationSamples > 0xffff) {
          
          SendTelephoneEventPacket(ended, _dtmfTimestamp,
                                   static_cast<WebRtc_UWord16>(0xffff), false);

          
          _dtmfTimestamp = captureTimeStamp;
          dtmfDurationSamples -= 0xffff;
          _dtmfLengthSamples -= 0xffff;

          return SendTelephoneEventPacket(
              ended,
              _dtmfTimestamp,
              static_cast<WebRtc_UWord16>(dtmfDurationSamples),
              false);
        } else {
          
          _dtmfEventFirstPacketSent = true;
          return SendTelephoneEventPacket(
              ended,
              _dtmfTimestamp,
              static_cast<WebRtc_UWord16>(dtmfDurationSamples),
              !_dtmfEventFirstPacketSent);
        }
      }
      return 0;
    }
    _sendAudioCritsect->Leave();
  }
  if (payloadSize == 0 || payloadData == NULL) {
    if (frameType == kFrameEmpty) {
      
      
      return 0;
    }
    return -1;
  }
  WebRtc_UWord8 dataBuffer[IP_PACKET_SIZE];
  bool markerBit = MarkerBit(frameType, payloadType);

  WebRtc_Word32 rtpHeaderLength = 0;
  WebRtc_UWord16 timestampOffset = 0;

  if (_REDPayloadType >= 0 && fragmentation && !markerBit &&
      fragmentation->fragmentationVectorSize > 1) {
    
    
    WebRtc_UWord32 oldTimeStamp = _rtpSender->Timestamp();
    rtpHeaderLength = _rtpSender->BuildRTPheader(dataBuffer, _REDPayloadType,
                                                 markerBit, captureTimeStamp);

    timestampOffset = WebRtc_UWord16(_rtpSender->Timestamp() - oldTimeStamp);
  } else {
    rtpHeaderLength = _rtpSender->BuildRTPheader(dataBuffer, payloadType,
                                                 markerBit, captureTimeStamp);
  }
  if (rtpHeaderLength <= 0) {
    return -1;
  }
  {
    CriticalSectionScoped cs(_sendAudioCritsect);

    
    if (_includeAudioLevelIndication) {
      dataBuffer[0] |= 0x10; 
      








      
      ModuleRTPUtility::AssignUWord16ToBuffer(dataBuffer+rtpHeaderLength,
                                              RTP_AUDIO_LEVEL_UNIQUE_ID);
      rtpHeaderLength += 2;

      
      const WebRtc_UWord8 length = 1;
      ModuleRTPUtility::AssignUWord16ToBuffer(dataBuffer+rtpHeaderLength,
                                              length);
      rtpHeaderLength += 2;

      
      const WebRtc_UWord8 id = _audioLevelIndicationID;
      const WebRtc_UWord8 len = 0;
      dataBuffer[rtpHeaderLength++] = (id << 4) + len;

      
      const WebRtc_UWord8 V = (frameType == kAudioFrameSpeech);
      WebRtc_UWord8 level = _audioLevel_dBov;
      dataBuffer[rtpHeaderLength++] = (V << 7) + level;

      
      ModuleRTPUtility::AssignUWord16ToBuffer(dataBuffer+rtpHeaderLength, 0);
      rtpHeaderLength += 2;
    }

    if(maxPayloadLength < rtpHeaderLength + payloadSize ) {
      
      return -1;
    }

    if (_REDPayloadType >= 0 &&  
        fragmentation &&
        fragmentation->fragmentationVectorSize > 1 &&
        !markerBit) {
      if (timestampOffset <= 0x3fff) {
        if(fragmentation->fragmentationVectorSize != 2) {
          
          return -1;
        }
        
        dataBuffer[rtpHeaderLength++] = 0x80 +
            fragmentation->fragmentationPlType[1];
        WebRtc_UWord32 blockLength = fragmentation->fragmentationLength[1];

        
        if(blockLength > 0x3ff) {  
          return -1;
        }
        WebRtc_UWord32 REDheader = (timestampOffset << 10) + blockLength;
        ModuleRTPUtility::AssignUWord24ToBuffer(dataBuffer + rtpHeaderLength,
                                                REDheader);
        rtpHeaderLength += 3;

        dataBuffer[rtpHeaderLength++] = fragmentation->fragmentationPlType[0];
        
        memcpy(dataBuffer+rtpHeaderLength,
               payloadData + fragmentation->fragmentationOffset[1],
               fragmentation->fragmentationLength[1]);

        
        memcpy(dataBuffer+rtpHeaderLength +
               fragmentation->fragmentationLength[1],
               payloadData + fragmentation->fragmentationOffset[0],
               fragmentation->fragmentationLength[0]);

        payloadSize = static_cast<WebRtc_UWord16>(
            fragmentation->fragmentationLength[0] +
            fragmentation->fragmentationLength[1]);
      } else {
        
        dataBuffer[rtpHeaderLength++] = fragmentation->fragmentationPlType[0];
        memcpy(dataBuffer+rtpHeaderLength,
               payloadData + fragmentation->fragmentationOffset[0],
               fragmentation->fragmentationLength[0]);

        payloadSize = static_cast<WebRtc_UWord16>(
            fragmentation->fragmentationLength[0]);
      }
    } else {
      if (fragmentation && fragmentation->fragmentationVectorSize > 0) {
        
        dataBuffer[rtpHeaderLength++] = fragmentation->fragmentationPlType[0];
        memcpy( dataBuffer+rtpHeaderLength,
                payloadData + fragmentation->fragmentationOffset[0],
                fragmentation->fragmentationLength[0]);

        payloadSize = static_cast<WebRtc_UWord16>(
            fragmentation->fragmentationLength[0]);
      } else {
        memcpy(dataBuffer+rtpHeaderLength, payloadData, payloadSize);
      }
    }
    _lastPayloadType = payloadType;
  }   
  return _rtpSender->SendToNetwork(dataBuffer,
                                   payloadSize,
                                   static_cast<WebRtc_UWord16>(rtpHeaderLength),
                                   -1,
                                   kAllowRetransmission);
}

WebRtc_Word32
RTPSenderAudio::SetAudioLevelIndicationStatus(const bool enable,
                                              const WebRtc_UWord8 ID)
{
    if(ID < 1 || ID > 14)
    {
        return -1;
    }
    CriticalSectionScoped cs(_sendAudioCritsect);

    _includeAudioLevelIndication = enable;
    _audioLevelIndicationID = ID;

    return 0;
}

WebRtc_Word32
RTPSenderAudio::AudioLevelIndicationStatus(bool& enable,
                                           WebRtc_UWord8& ID) const
{
    CriticalSectionScoped cs(_sendAudioCritsect);
    enable = _includeAudioLevelIndication;
    ID = _audioLevelIndicationID;
    return 0;
}

    
WebRtc_Word32
RTPSenderAudio::SetAudioLevel(const WebRtc_UWord8 level_dBov)
{
    if (level_dBov > 127)
    {
        return -1;
    }
    CriticalSectionScoped cs(_sendAudioCritsect);
    _audioLevel_dBov = level_dBov;
    return 0;
}

    
WebRtc_Word32
RTPSenderAudio::SetRED(const WebRtc_Word8 payloadType)
{
    if(payloadType < -1 )
    {
        return -1;
    }
    _REDPayloadType = payloadType;
    return 0;
}

    
WebRtc_Word32
RTPSenderAudio::RED(WebRtc_Word8& payloadType) const
{
    if(_REDPayloadType == -1)
    {
        
        return -1;
    }
    payloadType = _REDPayloadType;
    return 0;
}


WebRtc_Word32
RTPSenderAudio::SendTelephoneEvent(const WebRtc_UWord8 key,
                                   const WebRtc_UWord16 time_ms,
                                   const WebRtc_UWord8 level)
{
    
    if(_dtmfPayloadType < 0)
    {
        
        return -1;
    }
    return AddDTMF(key, time_ms, level);
}

WebRtc_Word32
RTPSenderAudio::SendTelephoneEventPacket(const bool ended,
                                         const WebRtc_UWord32 dtmfTimeStamp,
                                         const WebRtc_UWord16 duration,
                                         const bool markerBit)
{
    WebRtc_UWord8 dtmfbuffer[IP_PACKET_SIZE];
    WebRtc_UWord8 sendCount = 1;
    WebRtc_Word32 retVal = 0;

    if(ended)
    {
        
        sendCount = 3;
    }
    do
    {
        _sendAudioCritsect->Enter();

        
        _rtpSender->BuildRTPheader(dtmfbuffer, _dtmfPayloadType, markerBit, dtmfTimeStamp);

        
        dtmfbuffer[0] &= 0xe0;

        
        







        
        WebRtc_UWord8 R = 0x00;
        WebRtc_UWord8 volume = _dtmfLevel;

        
          WebRtc_UWord8 E = 0x00;

        if(ended)
        {
            E = 0x80;
        }

        
        dtmfbuffer[12] = _dtmfKey;
        dtmfbuffer[13] = E|R|volume;
        ModuleRTPUtility::AssignUWord16ToBuffer(dtmfbuffer+14, duration);

        _sendAudioCritsect->Leave();
        retVal = _rtpSender->SendToNetwork(dtmfbuffer, 4, 12, -1,
                                           kAllowRetransmission);
        sendCount--;

    }while (sendCount > 0 && retVal == 0);

    return retVal;
}
} 
