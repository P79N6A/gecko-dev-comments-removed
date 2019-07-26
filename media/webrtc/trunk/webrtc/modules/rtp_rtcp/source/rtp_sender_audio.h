









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_AUDIO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_AUDIO_H_

#include "rtp_rtcp_config.h"          
#include "common_types.h"             
#include "typedefs.h"

#include "dtmf_queue.h"
#include "rtp_utility.h"

#include "rtp_sender.h"

namespace webrtc {
class RTPSenderAudio: public DTMFqueue
{
public:
    RTPSenderAudio(const WebRtc_Word32 id, RtpRtcpClock* clock,
                   RTPSenderInterface* rtpSender);
    virtual ~RTPSenderAudio();

    WebRtc_Word32 RegisterAudioPayload(
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        const WebRtc_Word8 payloadType,
        const WebRtc_UWord32 frequency,
        const WebRtc_UWord8 channels,
        const WebRtc_UWord32 rate,
        ModuleRTPUtility::Payload*& payload);

    WebRtc_Word32 SendAudio(const FrameType frameType,
                            const WebRtc_Word8 payloadType,
                            const WebRtc_UWord32 captureTimeStamp,
                            const WebRtc_UWord8* payloadData,
                            const WebRtc_UWord32 payloadSize,
                            const RTPFragmentationHeader* fragmentation);

    
    WebRtc_Word32 SetAudioPacketSize(const WebRtc_UWord16 packetSizeSamples);

    
    
    WebRtc_Word32 SetAudioLevelIndicationStatus(const bool enable,
                                              const WebRtc_UWord8 ID);

    
    WebRtc_Word32 AudioLevelIndicationStatus(bool& enable,
                                           WebRtc_UWord8& ID) const;

    
    
    WebRtc_Word32 SetAudioLevel(const WebRtc_UWord8 level_dBov);

    
      WebRtc_Word32 SendTelephoneEvent(const WebRtc_UWord8 key,
                                   const WebRtc_UWord16 time_ms,
                                   const WebRtc_UWord8 level);

    bool SendTelephoneEventActive(WebRtc_Word8& telephoneEvent) const;

    void SetAudioFrequency(const WebRtc_UWord32 f);

    int AudioFrequency() const;

    
    WebRtc_Word32 SetRED(const WebRtc_Word8 payloadType);

    
    WebRtc_Word32 RED(WebRtc_Word8& payloadType) const;

    WebRtc_Word32 RegisterAudioCallback(RtpAudioFeedback* messagesCallback);

protected:
    WebRtc_Word32 SendTelephoneEventPacket(const bool ended,
                                         const WebRtc_UWord32 dtmfTimeStamp,
                                         const WebRtc_UWord16 duration,
                                         const bool markerBit); 

    bool MarkerBit(const FrameType frameType,
                   const WebRtc_Word8 payloadType);

private:
    WebRtc_Word32             _id;
    RtpRtcpClock&             _clock;
    RTPSenderInterface*     _rtpSender;
    CriticalSectionWrapper* _audioFeedbackCritsect;
    RtpAudioFeedback*   _audioFeedback;

    CriticalSectionWrapper*   _sendAudioCritsect;

    WebRtc_UWord32            _frequency;
    WebRtc_UWord16            _packetSizeSamples;

    
    bool              _dtmfEventIsOn;
    bool              _dtmfEventFirstPacketSent;
    WebRtc_Word8      _dtmfPayloadType;
    WebRtc_UWord32    _dtmfTimestamp;
    WebRtc_UWord8     _dtmfKey;
    WebRtc_UWord32    _dtmfLengthSamples;
    WebRtc_UWord8     _dtmfLevel;
    WebRtc_Word64     _dtmfTimeLastSent;
    WebRtc_UWord32    _dtmfTimestampLastSent;

    WebRtc_Word8      _REDPayloadType;

    
    bool              _inbandVADactive;
    WebRtc_Word8      _cngNBPayloadType;
    WebRtc_Word8      _cngWBPayloadType;
    WebRtc_Word8      _cngSWBPayloadType;
    WebRtc_Word8      _cngFBPayloadType;
    WebRtc_Word8      _lastPayloadType;

    
    bool            _includeAudioLevelIndication;
    WebRtc_UWord8     _audioLevelIndicationID;
    WebRtc_UWord8     _audioLevel_dBov;
};
} 

#endif 
