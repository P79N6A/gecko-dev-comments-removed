









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_AUDIO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_AUDIO_H_

#include <set>

#include "rtp_receiver.h"
#include "rtp_receiver_strategy.h"
#include "rtp_rtcp_defines.h"
#include "rtp_utility.h"
#include "scoped_ptr.h"
#include "typedefs.h"

namespace webrtc {
class CriticalSectionWrapper;
class RTPReceiver;


class RTPReceiverAudio : public RTPReceiverStrategy
{
public:
    RTPReceiverAudio(const WebRtc_Word32 id,
                     RTPReceiver* parent,
                     RtpAudioFeedback* incomingMessagesCallback);

    WebRtc_UWord32 AudioFrequency() const;

    
    WebRtc_Word32 SetTelephoneEventStatus(const bool enable,
                                        const bool forwardToDecoder,
                                        const bool detectEndOfTone);

    
    bool TelephoneEvent() const ;

    
    bool TelephoneEventForwardToDecoder() const ;

    
    bool TelephoneEventPayloadType(const WebRtc_Word8 payloadType) const;

    
    
    bool CNGPayloadType(const WebRtc_Word8 payloadType,
                        WebRtc_UWord32* frequency,
                        bool* cngPayloadTypeHasChanged);

    WebRtc_Word32 ParseRtpPacket(
        WebRtcRTPHeader* rtpHeader,
        const ModuleRTPUtility::PayloadUnion& specificPayload,
        const bool isRed,
        const WebRtc_UWord8* packet,
        const WebRtc_UWord16 packetLength,
        const WebRtc_Word64 timestampMs);

    WebRtc_Word32 GetFrequencyHz() const;

    RTPAliveType ProcessDeadOrAlive(WebRtc_UWord16 lastPayloadLength) const;

    bool PayloadIsCompatible(
        const ModuleRTPUtility::Payload& payload,
        const WebRtc_UWord32 frequency,
        const WebRtc_UWord8 channels,
        const WebRtc_UWord32 rate) const;

    void UpdatePayloadRate(
        ModuleRTPUtility::Payload* payload,
        const WebRtc_UWord32 rate) const;

    ModuleRTPUtility::Payload* CreatePayloadType(
          const char payloadName[RTP_PAYLOAD_NAME_SIZE],
          const WebRtc_Word8 payloadType,
          const WebRtc_UWord32 frequency,
          const WebRtc_UWord8 channels,
          const WebRtc_UWord32 rate);

    WebRtc_Word32 InvokeOnInitializeDecoder(
          RtpFeedback* callback,
          const WebRtc_Word32 id,
          const WebRtc_Word8 payloadType,
          const char payloadName[RTP_PAYLOAD_NAME_SIZE],
          const ModuleRTPUtility::PayloadUnion& specificPayload) const;

    
    
    void PossiblyRemoveExistingPayloadType(
        ModuleRTPUtility::PayloadTypeMap* payloadTypeMap,
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        const size_t payloadNameLength,
        const WebRtc_UWord32 frequency,
        const WebRtc_UWord8 channels,
        const WebRtc_UWord32 rate) const;

    
    
    void CheckPayloadChanged(
        const WebRtc_Word8 payloadType,
        ModuleRTPUtility::PayloadUnion* specificPayload,
        bool* shouldResetStatistics,
        bool* shouldDiscardChanges);
private:
    void SendTelephoneEvents(
        WebRtc_UWord8 numberOfNewEvents,
        WebRtc_UWord8 newEvents[MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS],
        WebRtc_UWord8 numberOfRemovedEvents,
        WebRtc_UWord8 removedEvents[MAX_NUMBER_OF_PARALLEL_TELEPHONE_EVENTS]);

    WebRtc_Word32 ParseAudioCodecSpecific(
        WebRtcRTPHeader* rtpHeader,
        const WebRtc_UWord8* payloadData,
        const WebRtc_UWord16 payloadLength,
        const ModuleRTPUtility::AudioPayload& audioSpecific,
        const bool isRED);

    WebRtc_Word32                      _id;
    RTPReceiver*                       _parent;
    scoped_ptr<CriticalSectionWrapper> _criticalSectionRtpReceiverAudio;

    WebRtc_UWord32                     _lastReceivedFrequency;

    bool                    _telephoneEvent;
    bool                    _telephoneEventForwardToDecoder;
    bool                    _telephoneEventDetectEndOfTone;
    WebRtc_Word8            _telephoneEventPayloadType;
    std::set<WebRtc_UWord8> _telephoneEventReported;

    WebRtc_Word8              _cngNBPayloadType;
    WebRtc_Word8              _cngWBPayloadType;
    WebRtc_Word8              _cngSWBPayloadType;
    WebRtc_Word8              _cngFBPayloadType;
    WebRtc_Word8              _cngPayloadType;

    
    WebRtc_Word8              _G722PayloadType;
    bool                      _lastReceivedG722;

    RtpAudioFeedback*         _cbAudioFeedback;
};
} 
#endif 
