









#ifndef WEBRTC_MODULES_VIDEO_CODING_VIDEO_CODING_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CODING_VIDEO_CODING_IMPL_H_

#include "modules/video_coding/main/interface/video_coding.h"

#include <vector>

#include "modules/video_coding/main/source/codec_database.h"
#include "modules/video_coding/main/source/frame_buffer.h"
#include "modules/video_coding/main/source/generic_decoder.h"
#include "modules/video_coding/main/source/generic_encoder.h"
#include "modules/video_coding/main/source/jitter_buffer.h"
#include "modules/video_coding/main/source/media_optimization.h"
#include "modules/video_coding/main/source/receiver.h"
#include "modules/video_coding/main/source/tick_time_base.h"
#include "modules/video_coding/main/source/timing.h"
#include "system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc
{

class VCMProcessTimer
{
public:
    VCMProcessTimer(WebRtc_UWord32 periodMs, TickTimeBase* clock)
        : _clock(clock),
          _periodMs(periodMs),
          _latestMs(_clock->MillisecondTimestamp()) {}
    WebRtc_UWord32 Period() const;
    WebRtc_UWord32 TimeUntilProcess() const;
    void Processed();

private:
    TickTimeBase*         _clock;
    WebRtc_UWord32        _periodMs;
    WebRtc_Word64         _latestMs;
};

enum VCMKeyRequestMode
{
    kKeyOnError,    
    kKeyOnKeyLoss,  
                    
    kKeyOnLoss,     
                    
};

class VideoCodingModuleImpl : public VideoCodingModule
{
public:
    VideoCodingModuleImpl(const WebRtc_Word32 id,
                          TickTimeBase* clock,
                          bool delete_clock_on_destroy);

    virtual ~VideoCodingModuleImpl();

    WebRtc_Word32 Id() const;

    
    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

    
    
    virtual WebRtc_Word32 TimeUntilNextProcess();

    virtual WebRtc_Word32 Process();

    



    
    virtual WebRtc_Word32 InitializeSender();

    
    virtual WebRtc_Word32 RegisterSendCodec(const VideoCodec* sendCodec,
                                            WebRtc_UWord32 numberOfCores,
                                            WebRtc_UWord32 maxPayloadSize);

    
    virtual WebRtc_Word32 SendCodec(VideoCodec* currentSendCodec) const;

    
    virtual VideoCodecType SendCodec() const;

    
    virtual WebRtc_Word32 RegisterExternalEncoder(VideoEncoder* externalEncoder,
                                                  WebRtc_UWord8 payloadType,
                                                  bool internalSource = false);

    
    virtual WebRtc_Word32 CodecConfigParameters(WebRtc_UWord8* buffer,
                                                WebRtc_Word32 size);

    
    virtual int Bitrate(unsigned int* bitrate) const;

    
    virtual int FrameRate(unsigned int* framerate) const;

    
    virtual WebRtc_Word32 SetChannelParameters(
        WebRtc_UWord32 availableBandWidth,
        WebRtc_UWord8 lossRate,
        WebRtc_UWord32 rtt);

    
    virtual WebRtc_Word32 SetReceiveChannelParameters(WebRtc_UWord32 rtt);

    
    
    virtual WebRtc_Word32 RegisterTransportCallback(
        VCMPacketizationCallback* transport);

    
    
    
    virtual WebRtc_Word32 RegisterSendStatisticsCallback(
        VCMSendStatisticsCallback* sendStats);

    
    
    virtual WebRtc_Word32 RegisterVideoQMCallback(
        VCMQMSettingsCallback* videoQMSettings);

    
    
    virtual WebRtc_Word32 RegisterProtectionCallback(
        VCMProtectionCallback* protection);

    
   virtual WebRtc_Word32 SetVideoProtection(VCMVideoProtection videoProtection,
                                            bool enable);

    
    virtual WebRtc_Word32 AddVideoFrame(
        const I420VideoFrame& videoFrame,
        const VideoContentMetrics* _contentMetrics = NULL,
        const CodecSpecificInfo* codecSpecificInfo = NULL);

    virtual WebRtc_Word32 IntraFrameRequest(int stream_index);

    
    virtual WebRtc_Word32 EnableFrameDropper(bool enable);

    
    virtual WebRtc_Word32 SentFrameCount(VCMFrameCount& frameCount) const;

    



    
    virtual WebRtc_Word32 InitializeReceiver();

    
    virtual WebRtc_Word32 RegisterReceiveCodec(const VideoCodec* receiveCodec,
                                               WebRtc_Word32 numberOfCores,
                                               bool requireKeyFrame = false);

    
    
    virtual WebRtc_Word32 RegisterExternalDecoder(VideoDecoder* externalDecoder,
                                                  WebRtc_UWord8 payloadType,
                                                  bool internalRenderTiming);

    
    
    virtual WebRtc_Word32 RegisterReceiveCallback(
        VCMReceiveCallback* receiveCallback);

    
    
    
    virtual WebRtc_Word32 RegisterReceiveStatisticsCallback(
        VCMReceiveStatisticsCallback* receiveStats);

    
    virtual WebRtc_Word32 RegisterFrameTypeCallback(
        VCMFrameTypeCallback* frameTypeCallback);

    
    virtual WebRtc_Word32 RegisterFrameStorageCallback(
        VCMFrameStorageCallback* frameStorageCallback);

    
    virtual WebRtc_Word32 RegisterPacketRequestCallback(
        VCMPacketRequestCallback* callback);

    
    
    virtual WebRtc_Word32 Decode(WebRtc_UWord16 maxWaitTimeMs = 200);

    
    
    virtual WebRtc_Word32 DecodeDualFrame(WebRtc_UWord16 maxWaitTimeMs = 200);

    
    virtual WebRtc_Word32 ResetDecoder();

    
    virtual WebRtc_Word32 ReceiveCodec(VideoCodec* currentReceiveCodec) const;

    
    virtual VideoCodecType ReceiveCodec() const;

    
    virtual WebRtc_Word32 IncomingPacket(const WebRtc_UWord8* incomingPayload,
                                         WebRtc_UWord32 payloadLength,
                                         const WebRtcRTPHeader& rtpInfo);

    
    
    virtual WebRtc_Word32 DecodeFromStorage(
        const EncodedVideoData& frameFromStorage);

    
    
    
    virtual WebRtc_Word32 SetMinimumPlayoutDelay(
        WebRtc_UWord32 minPlayoutDelayMs);

    
    virtual WebRtc_Word32 SetRenderDelay(WebRtc_UWord32 timeMS);

    
    virtual WebRtc_Word32 Delay() const;

    
    virtual WebRtc_Word32 ReceivedFrameCount(VCMFrameCount& frameCount) const;

    
    virtual WebRtc_UWord32 DiscardedPackets() const;


    

    
    virtual int SetSenderNackMode(SenderNackMode mode);

    
    virtual int SetSenderReferenceSelection(bool enable);

    
    virtual int SetSenderFEC(bool enable);

    
    virtual int SetSenderKeyFramePeriod(int periodMs);

    
    virtual int SetReceiverRobustnessMode(ReceiverRobustness robustnessMode,
                                          DecodeErrors errorMode);
    
    virtual int StartDebugRecording(const char* file_name_utf8);

    
    virtual int StopDebugRecording();

protected:
    WebRtc_Word32 Decode(const webrtc::VCMEncodedFrame& frame);
    WebRtc_Word32 RequestKeyFrame();
    WebRtc_Word32 RequestSliceLossIndication(
        const WebRtc_UWord64 pictureID) const;
    WebRtc_Word32 NackList(WebRtc_UWord16* nackList, WebRtc_UWord16& size);

private:
    WebRtc_Word32                       _id;
    TickTimeBase*                       clock_;
    bool                                delete_clock_on_destroy_;
    CriticalSectionWrapper*             _receiveCritSect;
    bool                                _receiverInited;
    VCMTiming                           _timing;
    VCMTiming                           _dualTiming;
    VCMReceiver                         _receiver;
    VCMReceiver                         _dualReceiver;
    VCMDecodedFrameCallback             _decodedFrameCallback;
    VCMDecodedFrameCallback             _dualDecodedFrameCallback;
    VCMFrameTypeCallback*               _frameTypeCallback;
    VCMFrameStorageCallback*            _frameStorageCallback;
    VCMReceiveStatisticsCallback*       _receiveStatsCallback;
    VCMPacketRequestCallback*           _packetRequestCallback;
    VCMGenericDecoder*                  _decoder;
    VCMGenericDecoder*                  _dualDecoder;
#ifdef DEBUG_DECODER_BIT_STREAM
    FILE*                               _bitStreamBeforeDecoder;
#endif
    VCMFrameBuffer                      _frameFromFile;
    VCMKeyRequestMode                   _keyRequestMode;
    bool                                _scheduleKeyRequest;

    CriticalSectionWrapper*             _sendCritSect; 
    VCMGenericEncoder*                  _encoder;
    VCMEncodedFrameCallback             _encodedFrameCallback;
    std::vector<FrameType>              _nextFrameTypes;
    VCMMediaOptimization                _mediaOpt;
    VideoCodecType                      _sendCodecType;
    VCMSendStatisticsCallback*          _sendStatsCallback;
    FILE*                               _encoderInputFile;
    VCMCodecDataBase                    _codecDataBase;
    VCMProcessTimer                     _receiveStatsTimer;
    VCMProcessTimer                     _sendStatsTimer;
    VCMProcessTimer                     _retransmissionTimer;
    VCMProcessTimer                     _keyRequestTimer;
};
} 
#endif 
