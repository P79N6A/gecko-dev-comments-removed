









#ifndef WEBRTC_MODULES_INTERFACE_VIDEO_CODING_H_
#define WEBRTC_MODULES_INTERFACE_VIDEO_CODING_H_

#include "common_video/interface/i420_video_frame.h"
#include "modules/interface/module.h"
#include "modules/interface/module_common_types.h"
#include "modules/video_coding/main/interface/video_coding_defines.h"

namespace webrtc
{

class TickTimeBase;
class VideoEncoder;
class VideoDecoder;
struct CodecSpecificInfo;

class VideoCodingModule : public Module
{
public:
    enum SenderNackMode {
        kNackNone,
        kNackAll,
        kNackSelective
    };

    enum ReceiverRobustness {
        kNone,
        kHardNack,
        kSoftNack,
        kDualDecoder,
        kReferenceSelection
    };

    enum DecodeErrors {
        kNoDecodeErrors,
        kAllowDecodeErrors
    };

    static VideoCodingModule* Create(const WebRtc_Word32 id);

    static VideoCodingModule* Create(const WebRtc_Word32 id,
                                     TickTimeBase* clock);

    static void Destroy(VideoCodingModule* module);

    
    
    
    static WebRtc_UWord8 NumberOfCodecs();

    
    
    
    
    
    
    
    
    static WebRtc_Word32 Codec(const WebRtc_UWord8 listId, VideoCodec* codec);

    
    
    
    
    
    
    
    
    static WebRtc_Word32 Codec(VideoCodecType codecType, VideoCodec* codec);

    



    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 InitializeSender() = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterSendCodec(const VideoCodec* sendCodec,
                                            WebRtc_UWord32 numberOfCores,
                                            WebRtc_UWord32 maxPayloadSize) = 0;

    
    
    
    
    
    
    
    virtual WebRtc_Word32 SendCodec(VideoCodec* currentSendCodec) const = 0;

    
    
    
    
    virtual VideoCodecType SendCodec() const = 0;

    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterExternalEncoder(VideoEncoder* externalEncoder,
                                                  WebRtc_UWord8 payloadType,
                                                  bool internalSource = false) = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 CodecConfigParameters(WebRtc_UWord8* buffer, WebRtc_Word32 size) = 0;

    
    
    
    
    virtual int Bitrate(unsigned int* bitrate) const = 0;

    
    
    
    
    virtual int FrameRate(unsigned int* framerate) const = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetChannelParameters(WebRtc_UWord32 availableBandWidth,
                                               WebRtc_UWord8 lossRate,
                                               WebRtc_UWord32 rtt) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetReceiveChannelParameters(WebRtc_UWord32 rtt) = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterTransportCallback(VCMPacketizationCallback* transport) = 0;

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterSendStatisticsCallback(
                                     VCMSendStatisticsCallback* sendStats) = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterVideoQMCallback(VCMQMSettingsCallback* videoQMSettings) = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterProtectionCallback(VCMProtectionCallback* protection) = 0;

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetVideoProtection(VCMVideoProtection videoProtection,
                                             bool enable) = 0;

    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 AddVideoFrame(
        const I420VideoFrame& videoFrame,
        const VideoContentMetrics* contentMetrics = NULL,
        const CodecSpecificInfo* codecSpecificInfo = NULL) = 0;

    
    
    
    
    virtual WebRtc_Word32 IntraFrameRequest(int stream_index) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 EnableFrameDropper(bool enable) = 0;

    
    virtual WebRtc_Word32 SentFrameCount(VCMFrameCount& frameCount) const = 0;

    



    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 InitializeReceiver() = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterReceiveCodec(const VideoCodec* receiveCodec,
                                               WebRtc_Word32 numberOfCores,
                                               bool requireKeyFrame = false) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterExternalDecoder(VideoDecoder* externalDecoder,
                                                  WebRtc_UWord8 payloadType,
                                                  bool internalRenderTiming) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterReceiveCallback(VCMReceiveCallback* receiveCallback) = 0;

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterReceiveStatisticsCallback(
                               VCMReceiveStatisticsCallback* receiveStats) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterFrameTypeCallback(
                                  VCMFrameTypeCallback* frameTypeCallback) = 0;

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterFrameStorageCallback(
                             VCMFrameStorageCallback* frameStorageCallback) = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterPacketRequestCallback(
                                        VCMPacketRequestCallback* callback) = 0;

    
    
    
    
    
    
    virtual WebRtc_Word32 Decode(WebRtc_UWord16 maxWaitTimeMs = 200) = 0;

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 DecodeDualFrame(WebRtc_UWord16 maxWaitTimeMs = 200) = 0;

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 DecodeFromStorage(const EncodedVideoData& frameFromStorage) = 0;

    
    
    
    
    virtual WebRtc_Word32 ResetDecoder() = 0;

    
    
    
    
    
    
    
    virtual WebRtc_Word32 ReceiveCodec(VideoCodec* currentReceiveCodec) const = 0;

    
    
    
    
    virtual VideoCodecType ReceiveCodec() const = 0;

    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 IncomingPacket(const WebRtc_UWord8* incomingPayload,
                                       WebRtc_UWord32 payloadLength,
                                       const WebRtcRTPHeader& rtpInfo) = 0;

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetMinimumPlayoutDelay(WebRtc_UWord32 minPlayoutDelayMs) = 0;

    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetRenderDelay(WebRtc_UWord32 timeMS) = 0;

    
    
    
    
    
    virtual WebRtc_Word32 Delay() const = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 ReceivedFrameCount(VCMFrameCount& frameCount) const = 0;

    
    
    
    
    virtual WebRtc_UWord32 DiscardedPackets() const = 0;


    

    
    
    
    
    
    
    virtual int SetSenderNackMode(SenderNackMode mode) = 0;

    
    
    
    
    
    
    virtual int SetSenderReferenceSelection(bool enable) = 0;

    
    
    
    
    
    
    virtual int SetSenderFEC(bool enable) = 0;

    
    
    
    
    
    
    virtual int SetSenderKeyFramePeriod(int periodMs) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual int SetReceiverRobustnessMode(ReceiverRobustness robustnessMode,
                                          DecodeErrors errorMode) = 0;

    
    virtual int StartDebugRecording(const char* file_name_utf8) = 0;

    
    virtual int StopDebugRecording() = 0;
};

} 

#endif
