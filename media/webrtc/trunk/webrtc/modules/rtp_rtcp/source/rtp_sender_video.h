









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_VIDEO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_VIDEO_H_

#include <list>

#include "typedefs.h"
#include "common_types.h"               
#include "rtp_rtcp_config.h"

#include "rtp_rtcp_defines.h"
#include "rtp_utility.h"

#include "video_codec_information.h"
#include "forward_error_correction.h"
#include "bitrate.h"
#include "rtp_sender.h"
#include "producer_fec.h"

namespace webrtc {
class CriticalSectionWrapper;
struct RtpPacket;

class RTPSenderVideo
{
public:
    RTPSenderVideo(const WebRtc_Word32 id, RtpRtcpClock* clock,
                   RTPSenderInterface* rtpSender);
    virtual ~RTPSenderVideo();

    virtual RtpVideoCodecTypes VideoCodecType() const;

    WebRtc_UWord16 FECPacketOverhead() const;

    WebRtc_Word32 RegisterVideoPayload(
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        const WebRtc_Word8 payloadType,
        const WebRtc_UWord32 maxBitRate,
        ModuleRTPUtility::Payload*& payload);

    WebRtc_Word32 SendVideo(const RtpVideoCodecTypes videoType,
                          const FrameType frameType,
                          const WebRtc_Word8 payloadType,
                          const uint32_t captureTimeStamp,
                          int64_t capture_time_ms,
                          const WebRtc_UWord8* payloadData,
                          const WebRtc_UWord32 payloadSize,
                          const RTPFragmentationHeader* fragmentation,
                          VideoCodecInformation* codecInfo,
                          const RTPVideoTypeHeader* rtpTypeHdr);

    WebRtc_Word32 SendRTPIntraRequest();

    void SetVideoCodecType(RtpVideoCodecTypes type);

    VideoCodecInformation* CodecInformationVideo();

    void SetMaxConfiguredBitrateVideo(const WebRtc_UWord32 maxBitrate);

    WebRtc_UWord32 MaxConfiguredBitrateVideo() const;

    
    WebRtc_Word32 SetGenericFECStatus(const bool enable,
                                    const WebRtc_UWord8 payloadTypeRED,
                                    const WebRtc_UWord8 payloadTypeFEC);

    WebRtc_Word32 GenericFECStatus(bool& enable,
                                 WebRtc_UWord8& payloadTypeRED,
                                 WebRtc_UWord8& payloadTypeFEC) const;

    WebRtc_Word32 SetFecParameters(const FecProtectionParams* delta_params,
                                   const FecProtectionParams* key_params);

    void ProcessBitrate();

    WebRtc_UWord32 VideoBitrateSent() const;
    WebRtc_UWord32 FecOverheadRate() const;

    int SelectiveRetransmissions() const;
    int SetSelectiveRetransmissions(uint8_t settings);

protected:
    virtual WebRtc_Word32 SendVideoPacket(WebRtc_UWord8* dataBuffer,
                                          const WebRtc_UWord16 payloadLength,
                                          const WebRtc_UWord16 rtpHeaderLength,
                                          int64_t capture_time_ms,
                                          StorageType storage,
                                          bool protect);

private:
    WebRtc_Word32 SendGeneric(const WebRtc_Word8 payloadType,
                            const uint32_t captureTimeStamp,
                            int64_t capture_time_ms,
                            const WebRtc_UWord8* payloadData,
                            const WebRtc_UWord32 payloadSize);

    WebRtc_Word32 SendVP8(const FrameType frameType,
                        const WebRtc_Word8 payloadType,
                        const uint32_t captureTimeStamp,
                        int64_t capture_time_ms,
                        const WebRtc_UWord8* payloadData,
                        const WebRtc_UWord32 payloadSize,
                        const RTPFragmentationHeader* fragmentation,
                        const RTPVideoTypeHeader* rtpTypeHdr);

private:
    WebRtc_Word32             _id;
    RTPSenderInterface&        _rtpSender;

    CriticalSectionWrapper*   _sendVideoCritsect;
    RtpVideoCodecTypes  _videoType;
    VideoCodecInformation*  _videoCodecInformation;
    WebRtc_UWord32            _maxBitrate;
    WebRtc_Word32             _retransmissionSettings;

    
    ForwardErrorCorrection  _fec;
    bool                    _fecEnabled;
    WebRtc_Word8              _payloadTypeRED;
    WebRtc_Word8              _payloadTypeFEC;
    unsigned int              _numberFirstPartition;
    FecProtectionParams delta_fec_params_;
    FecProtectionParams key_fec_params_;
    ProducerFec producer_fec_;

    
    
    Bitrate                   _fecOverheadRate;
    
    Bitrate                   _videoBitrate;
};
} 

#endif 
