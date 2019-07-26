









#include "rtp_receiver_audio.h"

#include <cassert> 
#include <cstring> 
#include <math.h>    

#include "critical_section_wrapper.h"

namespace webrtc {
RTPReceiverAudio::RTPReceiverAudio(const WebRtc_Word32 id):
    _id(id),
    _lastReceivedFrequency(8000),
    _telephoneEvent(false),
    _telephoneEventForwardToDecoder(false),
    _telephoneEventDetectEndOfTone(false),
    _telephoneEventPayloadType(-1),
    _cngNBPayloadType(-1),
    _cngWBPayloadType(-1),
    _cngSWBPayloadType(-1),
    _cngPayloadType(-1),
    _G722PayloadType(-1),
    _lastReceivedG722(false),
    _criticalSectionFeedback(CriticalSectionWrapper::CreateCriticalSection()),
    _cbAudioFeedback(NULL)
{
}

RTPReceiverAudio::~RTPReceiverAudio()
{
    delete _criticalSectionFeedback;
}

WebRtc_Word32
RTPReceiverAudio::RegisterIncomingAudioCallback(RtpAudioFeedback* incomingMessagesCallback)
{
    CriticalSectionScoped lock(_criticalSectionFeedback);
    _cbAudioFeedback = incomingMessagesCallback;
    return 0;
}

WebRtc_UWord32
RTPReceiverAudio::AudioFrequency() const
{
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
    _telephoneEvent= enable;
    _telephoneEventDetectEndOfTone = detectEndOfTone;
    _telephoneEventForwardToDecoder = forwardToDecoder;
    return 0;
}

 
bool
RTPReceiverAudio::TelephoneEvent() const
{
    return _telephoneEvent;
}


bool
RTPReceiverAudio::TelephoneEventForwardToDecoder() const
{
    return _telephoneEventForwardToDecoder;
}

bool
RTPReceiverAudio::TelephoneEventPayloadType(const WebRtc_Word8 payloadType) const
{
    return (_telephoneEventPayloadType == payloadType)?true:false;
}

bool
RTPReceiverAudio::CNGPayloadType(const WebRtc_Word8 payloadType,
                                 WebRtc_UWord32& frequency)
{
    
    if(_cngNBPayloadType == payloadType)
    {
        frequency = 8000;
        if ((_cngPayloadType != -1) &&(_cngPayloadType !=_cngNBPayloadType))
        {
            ResetStatistics();
        }
        _cngPayloadType = _cngNBPayloadType;
        return true;
    } else if(_cngWBPayloadType == payloadType)
    {
        
        if(_lastReceivedG722)
        {
            frequency = 8000;
        } else
        {
            frequency = 16000;
        }
        if ((_cngPayloadType != -1) &&(_cngPayloadType !=_cngWBPayloadType))
        {
            ResetStatistics();
        }
        _cngPayloadType = _cngWBPayloadType;
        return true;
    }else if(_cngSWBPayloadType == payloadType)
    {
        frequency = 32000;
        if ((_cngPayloadType != -1) &&(_cngPayloadType !=_cngSWBPayloadType))
        {
            ResetStatistics();
        }
        _cngPayloadType = _cngSWBPayloadType;
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




































ModuleRTPUtility::Payload* RTPReceiverAudio::RegisterReceiveAudioPayload(
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const WebRtc_Word8 payloadType,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate) {
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
    bool telephoneEventPacket = TelephoneEventPayloadType(rtpHeader->header.payloadType);

    if(payloadLength == 0)
    {
        return 0;
    }

    {
        CriticalSectionScoped lock(_criticalSectionFeedback);

        if(telephoneEventPacket)
        {
            
            






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

        if(_telephoneEvent && _cbAudioFeedback)
        {
            for (int n = 0; n < numberOfNewEvents; n++)
            {
                _cbAudioFeedback->OnReceivedTelephoneEvent(_id, newEvents[n], false);
            }
            if(_telephoneEventDetectEndOfTone)
            {
                for (int n = 0; n < numberOfRemovedEvents; n++)
                {
                    _cbAudioFeedback->OnReceivedTelephoneEvent(_id, removedEvents[n], true);
                }
            }
        }
    }
    if(! telephoneEventPacket )
    {
        _lastReceivedFrequency = audioSpecific.frequency;
    }

    
    WebRtc_UWord32 dummy;
    if(CNGPayloadType(rtpHeader->header.payloadType, dummy))
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
    if(isRED && !(payloadData[0] & 0x80))
    {
        
        rtpHeader->header.payloadType = payloadData[0];

        
        return CallbackOfReceivedPayloadData(payloadData+1,
                                             payloadLength-1,
                                             rtpHeader);
    }

    rtpHeader->type.Audio.channel = audioSpecific.channels;
    return CallbackOfReceivedPayloadData(payloadData, payloadLength, rtpHeader);
}
} 
