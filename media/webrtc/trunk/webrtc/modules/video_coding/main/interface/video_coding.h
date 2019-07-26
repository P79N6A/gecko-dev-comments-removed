









#ifndef WEBRTC_MODULES_INTERFACE_VIDEO_CODING_H_
#define WEBRTC_MODULES_INTERFACE_VIDEO_CODING_H_

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"

namespace webrtc
{

class Clock;
class VideoEncoder;
class VideoDecoder;
struct CodecSpecificInfo;

class EventFactory {
 public:
  virtual ~EventFactory() {}

  virtual EventWrapper* CreateEvent() = 0;
};

class EventFactoryImpl : public EventFactory {
 public:
  virtual ~EventFactoryImpl() {}

  virtual EventWrapper* CreateEvent() {
    return EventWrapper::Create();
  }
};


enum VCMDecodeErrorMode {
  kNoErrors,                
                            
  kSelectiveErrors,         
                            
                            
                            
  kWithErrors               
                            
                            
};

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

    static VideoCodingModule* Create(const int32_t id);

    static VideoCodingModule* Create(const int32_t id,
                                     Clock* clock,
                                     EventFactory* event_factory);

    static void Destroy(VideoCodingModule* module);

    
    
    
    static uint8_t NumberOfCodecs();

    
    
    
    
    
    
    
    
    static int32_t Codec(const uint8_t listId, VideoCodec* codec);

    
    
    
    
    
    
    
    
    static int32_t Codec(VideoCodecType codecType, VideoCodec* codec);

    



    
    
    
    
    
    
    
    
    virtual int32_t InitializeSender() = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t RegisterSendCodec(const VideoCodec* sendCodec,
                                            uint32_t numberOfCores,
                                            uint32_t maxPayloadSize) = 0;

    
    
    
    
    
    
    
    virtual int32_t SendCodec(VideoCodec* currentSendCodec) const = 0;

    
    
    
    
    virtual VideoCodecType SendCodec() const = 0;

    
    
    
    
    
    
    
    
    
    
    virtual int32_t RegisterExternalEncoder(VideoEncoder* externalEncoder,
                                                  uint8_t payloadType,
                                                  bool internalSource = false) = 0;

    
    
    
    
    
    
    
    
    virtual int32_t CodecConfigParameters(uint8_t* buffer, int32_t size) = 0;

    
    
    
    
    virtual int Bitrate(unsigned int* bitrate) const = 0;

    
    
    
    
    virtual int FrameRate(unsigned int* framerate) const = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t SetChannelParameters(uint32_t target_bitrate,
                                               uint8_t lossRate,
                                               uint32_t rtt) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual int32_t SetReceiveChannelParameters(uint32_t rtt) = 0;

    
    
    
    
    
    
    
    
    virtual int32_t RegisterTransportCallback(VCMPacketizationCallback* transport) = 0;

    
    
    
    
    
    
    
    
    
    virtual int32_t RegisterSendStatisticsCallback(
                                     VCMSendStatisticsCallback* sendStats) = 0;

    
    
    
    
    
    
    
    
    virtual int32_t RegisterVideoQMCallback(VCMQMSettingsCallback* videoQMSettings) = 0;

    
    
    
    
    
    
    
    
    virtual int32_t RegisterProtectionCallback(VCMProtectionCallback* protection) = 0;

    
    
    
    
    
    
    
    
    
    virtual int32_t SetVideoProtection(VCMVideoProtection videoProtection,
                                       bool enable) = 0;

    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t AddVideoFrame(
        const I420VideoFrame& videoFrame,
        const VideoContentMetrics* contentMetrics = NULL,
        const CodecSpecificInfo* codecSpecificInfo = NULL) = 0;

    
    
    
    
    virtual int32_t IntraFrameRequest(int stream_index) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual int32_t EnableFrameDropper(bool enable) = 0;

    
    virtual int32_t SentFrameCount(VCMFrameCount& frameCount) const = 0;

    



    
    
    
    
    
    
    
    
    
    virtual int32_t InitializeReceiver() = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t RegisterReceiveCodec(const VideoCodec* receiveCodec,
                                         int32_t numberOfCores,
                                         bool requireKeyFrame = false) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t RegisterExternalDecoder(VideoDecoder* externalDecoder,
                                                  uint8_t payloadType,
                                                  bool internalRenderTiming) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual int32_t RegisterReceiveCallback(VCMReceiveCallback* receiveCallback) = 0;

    
    
    
    
    
    
    
    
    
    virtual int32_t RegisterReceiveStatisticsCallback(
                               VCMReceiveStatisticsCallback* receiveStats) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual int32_t RegisterFrameTypeCallback(
                                  VCMFrameTypeCallback* frameTypeCallback) = 0;

    
    
    
    
    
    
    
    
    virtual int32_t RegisterPacketRequestCallback(
                                        VCMPacketRequestCallback* callback) = 0;

    
    
    
    
    
    
    virtual int32_t Decode(uint16_t maxWaitTimeMs = 200) = 0;

    
    virtual int RegisterRenderBufferSizeCallback(
        VCMRenderBufferSizeCallback* callback) = 0;

    
    
    
    
    
    
    
    
    
    virtual int32_t DecodeDualFrame(uint16_t maxWaitTimeMs = 200) = 0;

    
    
    
    
    virtual int32_t ResetDecoder() = 0;

    
    
    
    
    
    
    
    virtual int32_t ReceiveCodec(VideoCodec* currentReceiveCodec) const = 0;

    
    
    
    
    virtual VideoCodecType ReceiveCodec() const = 0;

    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t IncomingPacket(const uint8_t* incomingPayload,
                                       uint32_t payloadLength,
                                       const WebRtcRTPHeader& rtpInfo) = 0;

    
    
    
    
    
    
    
    
    
    virtual int32_t SetMinimumPlayoutDelay(uint32_t minPlayoutDelayMs) = 0;

    
    
    
    
    
    
    
    virtual int32_t SetRenderDelay(uint32_t timeMS) = 0;

    
    
    
    
    
    virtual int32_t Delay() const = 0;

    
    
    
    
    
    
    
    
    virtual int32_t ReceivedFrameCount(VCMFrameCount& frameCount) const = 0;

    
    
    
    
    virtual uint32_t DiscardedPackets() const = 0;


    

    
    
    
    
    
    
    virtual int SetSenderNackMode(SenderNackMode mode) = 0;

    
    
    
    
    
    
    virtual int SetSenderReferenceSelection(bool enable) = 0;

    
    
    
    
    
    
    virtual int SetSenderFEC(bool enable) = 0;

    
    
    
    
    
    
    virtual int SetSenderKeyFramePeriod(int periodMs) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual int SetReceiverRobustnessMode(ReceiverRobustness robustnessMode,
                                          VCMDecodeErrorMode errorMode) = 0;

    
    
    
    
    
    virtual void SetDecodeErrorMode(VCMDecodeErrorMode decode_error_mode) = 0;

    
    
    
    
    
    
    virtual void SetNackSettings(size_t max_nack_list_size,
                                 int max_packet_age_to_nack,
                                 int max_incomplete_time_ms) = 0;

    
    
    virtual int SetMinReceiverDelay(int desired_delay_ms) = 0;

    
    virtual int StartDebugRecording(const char* file_name_utf8) = 0;

    
    virtual int StopDebugRecording() = 0;
};

}  

#endif
