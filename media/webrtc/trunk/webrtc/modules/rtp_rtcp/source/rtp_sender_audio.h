









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_AUDIO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_AUDIO_H_

#include "webrtc/common_types.h"
#include "webrtc/modules/rtp_rtcp/source/dtmf_queue.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_rtcp_config.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_sender.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class RTPSenderAudio: public DTMFqueue
{
public:
    RTPSenderAudio(const int32_t id, Clock* clock,
                   RTPSenderInterface* rtpSender);
    virtual ~RTPSenderAudio();

    int32_t RegisterAudioPayload(
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        const int8_t payloadType,
        const uint32_t frequency,
        const uint8_t channels,
        const uint32_t rate,
        ModuleRTPUtility::Payload*& payload);

    int32_t SendAudio(const FrameType frameType,
                      const int8_t payloadType,
                      const uint32_t captureTimeStamp,
                      const uint8_t* payloadData,
                      const uint32_t payloadSize,
                      const RTPFragmentationHeader* fragmentation);

    
    int32_t SetAudioPacketSize(const uint16_t packetSizeSamples);

    
    
    int32_t SetAudioLevelIndicationStatus(const bool enable, const uint8_t ID);

    
    int32_t AudioLevelIndicationStatus(bool& enable, uint8_t& ID) const;

    
    
    int32_t SetAudioLevel(const uint8_t level_dBov);

    
      int32_t SendTelephoneEvent(const uint8_t key,
                                 const uint16_t time_ms,
                                 const uint8_t level);

    bool SendTelephoneEventActive(int8_t& telephoneEvent) const;

    void SetAudioFrequency(const uint32_t f);

    int AudioFrequency() const;

    
    int32_t SetRED(const int8_t payloadType);

    
    int32_t RED(int8_t& payloadType) const;

    int32_t RegisterAudioCallback(RtpAudioFeedback* messagesCallback);

protected:
    int32_t SendTelephoneEventPacket(const bool ended,
                                     const uint32_t dtmfTimeStamp,
                                     const uint16_t duration,
                                     const bool markerBit); 

    bool MarkerBit(const FrameType frameType,
                   const int8_t payloadType);

private:
    int32_t             _id;
    Clock*                    _clock;
    RTPSenderInterface*       _rtpSender;
    CriticalSectionWrapper*   _audioFeedbackCritsect;
    RtpAudioFeedback*         _audioFeedback;

    CriticalSectionWrapper*   _sendAudioCritsect;

    uint32_t            _frequency;
    uint16_t            _packetSizeSamples;

    
    bool              _dtmfEventIsOn;
    bool              _dtmfEventFirstPacketSent;
    int8_t      _dtmfPayloadType;
    uint32_t    _dtmfTimestamp;
    uint8_t     _dtmfKey;
    uint32_t    _dtmfLengthSamples;
    uint8_t     _dtmfLevel;
    int64_t     _dtmfTimeLastSent;
    uint32_t    _dtmfTimestampLastSent;

    int8_t      _REDPayloadType;

    
    bool              _inbandVADactive;
    int8_t      _cngNBPayloadType;
    int8_t      _cngWBPayloadType;
    int8_t      _cngSWBPayloadType;
    int8_t      _cngFBPayloadType;
    int8_t      _lastPayloadType;

    
    bool            _includeAudioLevelIndication;
    uint8_t     _audioLevelIndicationID;
    uint8_t     _audioLevel_dBov;
};
}  

#endif 
