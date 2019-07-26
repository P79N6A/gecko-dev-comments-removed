









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_VIDEO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_VIDEO_H_

#include <list>

#include "webrtc/common_types.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/bitrate.h"
#include "webrtc/modules/rtp_rtcp/source/forward_error_correction.h"
#include "webrtc/modules/rtp_rtcp/source/producer_fec.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_rtcp_config.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_sender.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/modules/rtp_rtcp/source/video_codec_information.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class CriticalSectionWrapper;
struct RtpPacket;

class RTPSenderVideo
{
public:
    RTPSenderVideo(const int32_t id, Clock* clock,
                   RTPSenderInterface* rtpSender);
    virtual ~RTPSenderVideo();

    virtual RtpVideoCodecTypes VideoCodecType() const;

    uint16_t FECPacketOverhead() const;

    int32_t RegisterVideoPayload(
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        const int8_t payloadType,
        const uint32_t maxBitRate,
        ModuleRTPUtility::Payload*& payload);

    int32_t SendVideo(const RtpVideoCodecTypes videoType,
                      const FrameType frameType,
                      const int8_t payloadType,
                      const uint32_t captureTimeStamp,
                      int64_t capture_time_ms,
                      const uint8_t* payloadData,
                      const uint32_t payloadSize,
                      const RTPFragmentationHeader* fragmentation,
                      VideoCodecInformation* codecInfo,
                      const RTPVideoTypeHeader* rtpTypeHdr);

    int32_t SendRTPIntraRequest();

    void SetVideoCodecType(RtpVideoCodecTypes type);

    VideoCodecInformation* CodecInformationVideo();

    void SetMaxConfiguredBitrateVideo(const uint32_t maxBitrate);

    uint32_t MaxConfiguredBitrateVideo() const;

    
    int32_t SetGenericFECStatus(const bool enable,
                                const uint8_t payloadTypeRED,
                                const uint8_t payloadTypeFEC);

    int32_t GenericFECStatus(bool& enable,
                             uint8_t& payloadTypeRED,
                             uint8_t& payloadTypeFEC) const;

    int32_t SetFecParameters(const FecProtectionParams* delta_params,
                             const FecProtectionParams* key_params);

    void ProcessBitrate();

    uint32_t VideoBitrateSent() const;
    uint32_t FecOverheadRate() const;

    int SelectiveRetransmissions() const;
    int SetSelectiveRetransmissions(uint8_t settings);

protected:
    virtual int32_t SendVideoPacket(uint8_t* dataBuffer,
                                    const uint16_t payloadLength,
                                    const uint16_t rtpHeaderLength,
                                    const uint32_t capture_timestamp,
                                    int64_t capture_time_ms,
                                    StorageType storage,
                                    bool protect);

private:
    int32_t SendGeneric(const FrameType frame_type,
                        const int8_t payload_type,
                        const uint32_t capture_timestamp,
                        int64_t capture_time_ms,
                        const uint8_t* payload, const uint32_t size);

    int32_t SendVP8(const FrameType frameType,
                    const int8_t payloadType,
                    const uint32_t captureTimeStamp,
                    int64_t capture_time_ms,
                    const uint8_t* payloadData,
                    const uint32_t payloadSize,
                    const RTPFragmentationHeader* fragmentation,
                    const RTPVideoTypeHeader* rtpTypeHdr);

private:
    int32_t             _id;
    RTPSenderInterface&        _rtpSender;

    CriticalSectionWrapper*   _sendVideoCritsect;
    RtpVideoCodecTypes  _videoType;
    VideoCodecInformation*  _videoCodecInformation;
    uint32_t            _maxBitrate;
    int32_t             _retransmissionSettings;

    
    ForwardErrorCorrection  _fec;
    bool                    _fecEnabled;
    int8_t              _payloadTypeRED;
    int8_t              _payloadTypeFEC;
    unsigned int              _numberFirstPartition;
    FecProtectionParams delta_fec_params_;
    FecProtectionParams key_fec_params_;
    ProducerFec producer_fec_;

    
    
    Bitrate                   _fecOverheadRate;
    
    Bitrate                   _videoBitrate;
};
}  

#endif 
