









#ifndef WEBRTC_MODULES_VIDEO_CODING_GENERIC_ENCODER_H_
#define WEBRTC_MODULES_VIDEO_CODING_GENERIC_ENCODER_H_

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"

#include <stdio.h>

#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
class CriticalSectionWrapper;

namespace media_optimization {
class MediaOptimization;
}  




class VCMEncodedFrameCallback : public EncodedImageCallback
{
public:
    VCMEncodedFrameCallback(EncodedImageCallback* post_encode_callback);
    virtual ~VCMEncodedFrameCallback();

  void SetCritSect(CriticalSectionWrapper* critSect);

    


    int32_t Encoded(
        EncodedImage& encodedImage,
        const CodecSpecificInfo* codecSpecificInfo = NULL,
        const RTPFragmentationHeader* fragmentationHeader = NULL);
    


    int32_t SetTransportCallback(VCMPacketizationCallback* transport);
    


    void SetMediaOpt (media_optimization::MediaOptimization* mediaOpt);

    void SetPayloadType(uint8_t payloadType) { _payloadType = payloadType; };
    void SetInternalSource(bool internalSource) { _internalSource = internalSource; };

private:
    VCMPacketizationCallback* _sendCallback;
    CriticalSectionWrapper* _critSect;
    media_optimization::MediaOptimization* _mediaOpt;
    uint8_t _payloadType;
    bool _internalSource;

    EncodedImageCallback* post_encode_callback_;

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
