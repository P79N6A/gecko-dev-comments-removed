









#ifndef WEBRTC_MODULES_VIDEO_CODING_GENERIC_ENCODER_H_
#define WEBRTC_MODULES_VIDEO_CODING_GENERIC_ENCODER_H_

#include "video_codec_interface.h"

#include <stdio.h>

namespace webrtc
{

namespace media_optimization {
class VCMMediaOptimization;
}  




class VCMEncodedFrameCallback : public EncodedImageCallback
{
public:
    VCMEncodedFrameCallback();
    virtual ~VCMEncodedFrameCallback();

    


    int32_t Encoded(
        EncodedImage& encodedImage,
        const CodecSpecificInfo* codecSpecificInfo = NULL,
        const RTPFragmentationHeader* fragmentationHeader = NULL);
    


    uint32_t EncodedBytes();
    


    int32_t SetTransportCallback(VCMPacketizationCallback* transport);
    


    void SetMediaOpt (media_optimization::VCMMediaOptimization* mediaOpt);

    void SetPayloadType(uint8_t payloadType) { _payloadType = payloadType; };
    void SetCodecType(VideoCodecType codecType) {_codecType = codecType;};
    void SetInternalSource(bool internalSource) { _internalSource = internalSource; };

private:
    



    static void CopyCodecSpecific(const CodecSpecificInfo& info,
                                  RTPVideoHeader** rtp);

    VCMPacketizationCallback* _sendCallback;
    media_optimization::VCMMediaOptimization* _mediaOpt;
    uint32_t _encodedBytes;
    uint8_t _payloadType;
    VideoCodecType _codecType;
    bool _internalSource;
#ifdef DEBUG_ENCODER_BIT_STREAM
    FILE* _bitStreamAfterEncoder;
#endif
};





class VCMGenericEncoder
{
    friend class VCMCodecDataBase;
public:
    VCMGenericEncoder(VideoEncoder& encoder, bool internalSource = false);
    ~VCMGenericEncoder();
    


    int32_t Release();
    


    int32_t InitEncode(const VideoCodec* settings,
                       int32_t numberOfCores,
                       uint32_t maxPayloadSize);
    






    int32_t Encode(const I420VideoFrame& inputFrame,
                   const CodecSpecificInfo* codecSpecificInfo,
                   const std::vector<FrameType>& frameTypes);
    



    int32_t SetRates(uint32_t target_bitrate, uint32_t frameRate);
    


    int32_t SetChannelParameters(int32_t packetLoss, int rtt);
    int32_t CodecConfigParameters(uint8_t* buffer, int32_t size);
    



    int32_t RegisterEncodeCallback(
        VCMEncodedFrameCallback* VCMencodedFrameCallback);
    


    uint32_t BitRate() const;
     


    uint32_t FrameRate() const;

    int32_t SetPeriodicKeyFrames(bool enable);

    int32_t RequestFrame(const std::vector<FrameType>& frame_types);

    bool InternalSource() const;

private:
    VideoEncoder&               _encoder;
    VideoCodecType              _codecType;
    VCMEncodedFrameCallback*    _VCMencodedFrameCallback;
    uint32_t                    _bitRate;
    uint32_t                    _frameRate;
    bool                        _internalSource;
}; 

} 

#endif 
