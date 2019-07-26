









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_AUDIO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_AUDIO_H_

#include <set>

#include "rtp_rtcp_defines.h"
#include "rtp_utility.h"

#include "typedefs.h"

namespace webrtc {
class CriticalSectionWrapper;

class RTPReceiverAudio
{
public:
    RTPReceiverAudio(const WebRtc_Word32 id);
    virtual ~RTPReceiverAudio();

    WebRtc_Word32 RegisterIncomingAudioCallback(RtpAudioFeedback* incomingMessagesCallback);

    ModuleRTPUtility::Payload* RegisterReceiveAudioPayload(
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        const WebRtc_Word8 payloadType,
        const WebRtc_UWord32 frequency,
        const WebRtc_UWord8 channels,
        const WebRtc_UWord32 rate);

    WebRtc_UWord32 AudioFrequency() const;

    
    WebRtc_Word32 SetTelephoneEventStatus(const bool enable,
                                        const bool forwardToDecoder,
                                        const bool detectEndOfTone);

    
    bool TelephoneEvent() const ;

    
    bool TelephoneEventForwardToDecoder() const ;

    
    bool TelephoneEventPayloadType(const WebRtc_Word8 payloadType) const;

    
    bool CNGPayloadType(const WebRtc_Word8 payloadType, WebRtc_UWord32& frequency);

    WebRtc_Word32 ParseAudioCodecSpecific(WebRtcRTPHeader* rtpHeader,
                                        const WebRtc_UWord8* payloadData,
                                        const WebRtc_UWord16 payloadLength,
                                        const ModuleRTPUtility::AudioPayload& audioSpecific,
                                        const bool isRED);

    virtual WebRtc_Word32 ResetStatistics() = 0;

protected:
    virtual WebRtc_Word32 CallbackOfReceivedPayloadData(const WebRtc_UWord8* payloadData,
                                                      const WebRtc_UWord16 payloadSize,
                                                      const WebRtcRTPHeader* rtpHeader) = 0;
private:
    WebRtc_Word32             _id;

    WebRtc_UWord32            _lastReceivedFrequency;

    bool                    _telephoneEvent;
    bool                    _telephoneEventForwardToDecoder;
    bool                    _telephoneEventDetectEndOfTone;
    WebRtc_Word8            _telephoneEventPayloadType;
    std::set<WebRtc_UWord8> _telephoneEventReported;

    WebRtc_Word8              _cngNBPayloadType;
    WebRtc_Word8              _cngWBPayloadType;
    WebRtc_Word8              _cngSWBPayloadType;
    WebRtc_Word8                _cngPayloadType;

    
    WebRtc_Word8              _G722PayloadType;
    bool                    _lastReceivedG722;

    CriticalSectionWrapper* _criticalSectionFeedback;
    RtpAudioFeedback*   _cbAudioFeedback;
};
} 
#endif 
