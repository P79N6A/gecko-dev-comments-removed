









#include "rtp_receiver_audio.h"

#include <cassert> 
#include <cstring> 
#include <math.h>    

#include "critical_section_wrapper.h"
#include "rtp_receiver.h"
#include "trace.h"

namespace webrtc {
RTPReceiverAudio::RTPReceiverAudio(const WebRtc_Word32 id,
                                   RTPReceiver* parent,
                                   RtpAudioFeedback* incomingMessagesCallback)
  : _id(id),
    _parent(parent),
    _criticalSectionRtpReceiverAudio(
        CriticalSectionWrapper::CreateCriticalSection()),
    _lastReceivedFrequency(8000),
    _telephoneEvent(false),
    _telephoneEventForwardToDecoder(false),
    _telephoneEventDetectEndOfTone(false),
    _telephoneEventPayloadType(-1),
    _cngNBPayloadType(-1),
    _cngWBPayloadType(-1),
    _cngSWBPayloadType(-1),
    _cngFBPayloadType(-1),
    _cngPayloadType(-1),
    _G722PayloadType(-1),
    _lastReceivedG722(false),
    _cbAudioFeedback(incomingMessagesCallback)
{
  last_payload_.Audio.channels = 1;
}

WebRtc_UWord32
RTPReceiverAudio::AudioFrequency() const
{
    CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());
    if(_lastReceivedG722)
    {
        return 8000;
    }
    return _lastReceivedFrequency;
}


WebRtc_Word32
RTPReceiverAudio::SetTelephoneEventStatus(const bool enable,
                                          const bool forwardToDecoder,
                                          const bool detectEndOfTone)
{
    CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());
    _telephoneEvent= enable;
    _telephoneEventDetectEndOfTone = detectEndOfTone;
    _telephoneEventForwardToDecoder = forwardToDecoder;
    return 0;
}

 
bool
RTPReceiverAudio::TelephoneEvent() const
{
    CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());
    return _telephoneEvent;
}


bool
RTPReceiverAudio::TelephoneEventForwardToDecoder() const
{
    CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());
    return _telephoneEventForwardToDecoder;
}

bool
RTPReceiverAudio::TelephoneEventPayloadType(const WebRtc_Word8 payloadType) const
{
    CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());
    return (_telephoneEventPayloadType == payloadType)?true:false;
}

bool
RTPReceiverAudio::CNGPayloadType(const WebRtc_Word8 payloadType,
                                 WebRtc_UWord32* frequency,
                                 bool* cngPayloadTypeHasChanged)
{
    CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());
    *cngPayloadTypeHasChanged = false;

    
    if(_cngNBPayloadType == payloadType)
    {
        *frequency = 8000;
        if ((_cngPayloadType != -1) &&(_cngPayloadType !=_cngNBPayloadType))
            *cngPayloadTypeHasChanged = true;

        _cngPayloadType = _cngNBPayloadType;
        return true;
    } else if(_cngWBPayloadType == payloadType)
    {
        
        if(_lastReceivedG722)
        {
            *frequency = 8000;
        } else
        {
            *frequency = 16000;
        }
        if ((_cngPayloadType != -1) &&(_cngPayloadType !=_cngWBPayloadType))
            *cngPayloadTypeHasChanged = true;
        _cngPayloadType = _cngWBPayloadType;
        return true;
    }else if(_cngSWBPayloadType == payloadType)
    {
        *frequency = 32000;
        if ((_cngPayloadType != -1) &&(_cngPayloadType !=_cngSWBPayloadType))
            *cngPayloadTypeHasChanged = true;
        _cngPayloadType = _cngSWBPayloadType;
        return true;
    }else if(_cngFBPayloadType == payloadType)
    {
        *frequency = 48000;
        if ((_cngPayloadType != -1) &&(_cngPayloadType !=_cngFBPayloadType))
            *cngPayloadTypeHasChanged = true;
        _cngPayloadType = _cngFBPayloadType;
        return true;
    }else
    {
        
        if(_G722PayloadType == payloadType)
        {
            _lastReceivedG722 = true;
        }else
        {
            _lastReceivedG722 = false;
        }
    }
    return false;
}




































ModuleRTPUtility::Payload* RTPReceiverAudio::CreatePayloadType(
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const WebRtc_Word8 payloadType,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate) {
  CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());

  if (ModuleRTPUtility::StringCompare(payloadName, "telephone-event", 15)) {
    _telephoneEventPayloadType = payloadType;
  }
  if (ModuleRTPUtility::StringCompare(payloadName, "cn", 2)) {
    
    if(frequency == 8000){
      _cngNBPayloadType = payloadType;
    } else if(frequency == 16000) {
      _cngWBPayloadType = payloadType;
    } else if(frequency == 32000) {
      _cngSWBPayloadType = payloadType;
    } else if(frequency == 48000) {
      _cngFBPayloadType = payloadType;
    } else {
      assert(false);
      return NULL;
    }
  }

  ModuleRTPUtility::Payload* payload = new ModuleRTPUtility::Payload;
  payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
  strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
  payload->typeSpecific.Audio.frequency = frequency;
  payload->typeSpecific.Audio.channels = channels;
  payload->typeSpecific.Audio.rate = rate;
  payload->audio = true;
  return payload;
}

void RTPReceiverAudio::SendTelephoneEvents(
    WebRtc_UWord8 numberOfNewEvents,
    WebRtc_UWord8 newEvents[MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS],
    WebRtc_UWord8 numberOfRemovedEvents,
    WebRtc_UWord8 removedEvents[MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS]) {

    
    
    bool telephoneEvent;
    bool telephoneEventDetectEndOfTone;
    {
      CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());
      telephoneEvent = _telephoneEvent;
      telephoneEventDetectEndOfTone = _telephoneEventDetectEndOfTone;
    }
    if (telephoneEvent) {
        for (int n = 0; n < numberOfNewEvents; ++n) {
            _cbAudioFeedback->OnReceivedTelephoneEvent(
                _id, newEvents[n], false);
        }
        if (telephoneEventDetectEndOfTone) {
            for (int n = 0; n < numberOfRemovedEvents; ++n) {
                _cbAudioFeedback->OnReceivedTelephoneEvent(
                    _id, removedEvents[n], true);
            }
        }
    }
}

WebRtc_Word32 RTPReceiverAudio::ParseRtpPacket(
    WebRtcRTPHeader* rtpHeader,
    const ModuleRTPUtility::PayloadUnion& specificPayload,
    const bool isRed,
    const WebRtc_UWord8* packet,
    const WebRtc_UWord16 packetLength,
    const WebRtc_Word64 timestampMs) {

    const WebRtc_UWord8* payloadData =
        ModuleRTPUtility::GetPayloadData(rtpHeader, packet);
    const WebRtc_UWord16 payloadDataLength =
        ModuleRTPUtility::GetPayloadDataLength(rtpHeader, packetLength);

    return ParseAudioCodecSpecific(rtpHeader, payloadData, payloadDataLength,
                                   specificPayload.Audio, isRed);
}

WebRtc_Word32 RTPReceiverAudio::GetFrequencyHz() const {
  return AudioFrequency();
}

RTPAliveType RTPReceiverAudio::ProcessDeadOrAlive(
    WebRtc_UWord16 lastPayloadLength) const {

    
    
    if(lastPayloadLength < 10)  
    {
        return kRtpNoRtp;
    } else
    {
        return kRtpDead;
    }
}

bool RTPReceiverAudio::PayloadIsCompatible(
    const ModuleRTPUtility::Payload& payload,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate) const {
  return
      payload.audio &&
      payload.typeSpecific.Audio.frequency == frequency &&
      payload.typeSpecific.Audio.channels == channels &&
      (payload.typeSpecific.Audio.rate == rate ||
          payload.typeSpecific.Audio.rate == 0 || rate == 0);
}

void RTPReceiverAudio::UpdatePayloadRate(
    ModuleRTPUtility::Payload* payload,
    const WebRtc_UWord32 rate) const {
  payload->typeSpecific.Audio.rate = rate;
}

void RTPReceiverAudio::PossiblyRemoveExistingPayloadType(
    ModuleRTPUtility::PayloadTypeMap* payloadTypeMap,
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const size_t payloadNameLength,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate) const {
  ModuleRTPUtility::PayloadTypeMap::iterator audio_it = payloadTypeMap->begin();
  while (audio_it != payloadTypeMap->end()) {
    ModuleRTPUtility::Payload* payload = audio_it->second;
    size_t nameLength = strlen(payload->name);

    if (payloadNameLength == nameLength &&
        ModuleRTPUtility::StringCompare(payload->name,
                                        payloadName, payloadNameLength)) {
      
      
      if (payload->audio) {
        if (payload->typeSpecific.Audio.frequency == frequency &&
            (payload->typeSpecific.Audio.rate == rate ||
                payload->typeSpecific.Audio.rate == 0 || rate == 0) &&
                payload->typeSpecific.Audio.channels == channels) {
          
          delete payload;
          payloadTypeMap->erase(audio_it);
          break;
        }
      } else if(ModuleRTPUtility::StringCompare(payloadName,"red",3)) {
        delete payload;
        payloadTypeMap->erase(audio_it);
        break;
      }
    }
    audio_it++;
  }
}

void RTPReceiverAudio::CheckPayloadChanged(
    const WebRtc_Word8 payloadType,
    ModuleRTPUtility::PayloadUnion* specificPayload,
    bool* shouldResetStatistics,
    bool* shouldDiscardChanges) {
  *shouldDiscardChanges = false;
  *shouldResetStatistics = false;

  if (TelephoneEventPayloadType(payloadType)) {
    
    *shouldDiscardChanges = true;
    return;
  }
  
  bool cngPayloadTypeHasChanged = false;
  bool isCngPayloadType = CNGPayloadType(
      payloadType, &specificPayload->Audio.frequency,
      &cngPayloadTypeHasChanged);

  *shouldResetStatistics = cngPayloadTypeHasChanged;

  if (isCngPayloadType) {
    
    *shouldDiscardChanges = true;
    return;
  }
}

WebRtc_Word32 RTPReceiverAudio::InvokeOnInitializeDecoder(
      RtpFeedback* callback,
      const WebRtc_Word32 id,
      const WebRtc_Word8 payloadType,
      const char payloadName[RTP_PAYLOAD_NAME_SIZE],
      const ModuleRTPUtility::PayloadUnion& specificPayload) const {
  if (-1 == callback->OnInitializeDecoder(
      id, payloadType, payloadName, specificPayload.Audio.frequency,
      specificPayload.Audio.channels, specificPayload.Audio.rate)) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, id,
                 "Failed to create video decoder for payload type:%d",
                 payloadType);
    return -1;
  }
  return 0;
}


WebRtc_Word32
RTPReceiverAudio::ParseAudioCodecSpecific(WebRtcRTPHeader* rtpHeader,
                                          const WebRtc_UWord8* payloadData,
                                          const WebRtc_UWord16 payloadLength,
                                          const ModuleRTPUtility::AudioPayload& audioSpecific,
                                          const bool isRED)
{
    WebRtc_UWord8 newEvents[MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS];
    WebRtc_UWord8 removedEvents[MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS];
    WebRtc_UWord8 numberOfNewEvents = 0;
    WebRtc_UWord8 numberOfRemovedEvents = 0;

    if(payloadLength == 0)
    {
        return 0;
    }

    bool telephoneEventPacket = TelephoneEventPayloadType(rtpHeader->header.payloadType);
    if(telephoneEventPacket)
    {
        CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());

        
        






        if(payloadLength % 4 != 0)
        {
            return -1;
        }
        WebRtc_UWord8 numberOfEvents = payloadLength / 4;

        
        if(numberOfEvents >= MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS)
        {
            numberOfEvents = MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS;
        }
        for (int n = 0; n < numberOfEvents; n++)
        {
            bool end = (payloadData[(4*n)+1] & 0x80)? true:false;

            std::set<WebRtc_UWord8>::iterator event =
                _telephoneEventReported.find(payloadData[4*n]);

            if(event != _telephoneEventReported.end())
            {
                
                if(end)
                {
                    removedEvents[numberOfRemovedEvents]= payloadData[4*n];
                    numberOfRemovedEvents++;
                    _telephoneEventReported.erase(payloadData[4*n]);
                }
            }else
            {
                if(end)
                {
                    
                }else
                {
                    newEvents[numberOfNewEvents] = payloadData[4*n];
                    numberOfNewEvents++;
                    _telephoneEventReported.insert(payloadData[4*n]);
                }
            }
        }

        
        

        
    }

    
    SendTelephoneEvents(numberOfNewEvents, newEvents, numberOfRemovedEvents,
                        removedEvents);

    {
        CriticalSectionScoped lock(_criticalSectionRtpReceiverAudio.get());

        if(! telephoneEventPacket )
        {
            _lastReceivedFrequency = audioSpecific.frequency;
        }

        
        WebRtc_UWord32 ignored;
        bool alsoIgnored;
        if(CNGPayloadType(rtpHeader->header.payloadType, &ignored, &alsoIgnored))
        {
            rtpHeader->type.Audio.isCNG=true;
            rtpHeader->frameType = kAudioFrameCN;
        }else
        {
            rtpHeader->frameType = kAudioFrameSpeech;
            rtpHeader->type.Audio.isCNG=false;
        }

        
        if(telephoneEventPacket)
        {
            if(!_telephoneEventForwardToDecoder)
            {
                
                return 0;
            }
            std::set<WebRtc_UWord8>::iterator first =
                _telephoneEventReported.begin();
            if(first != _telephoneEventReported.end() && *first > 15)
            {
                
                return 0;
            }
        }
    }
    if(isRED && !(payloadData[0] & 0x80))
    {
        
        rtpHeader->header.payloadType = payloadData[0];

        
        return _parent->CallbackOfReceivedPayloadData(payloadData+1,
                                                      payloadLength-1,
                                                      rtpHeader);
    }

    rtpHeader->type.Audio.channel = audioSpecific.channels;
    return _parent->CallbackOfReceivedPayloadData(
        payloadData, payloadLength, rtpHeader);
}
} 
