









#ifndef WEBRTC_MODULES_VIDEO_CODING_GENERIC_ENCODER_H_
#define WEBRTC_MODULES_VIDEO_CODING_GENERIC_ENCODER_H_

#include "video_codec_interface.h"

#include <stdio.h>

namespace webrtc
{

class VCMMediaOptimization;




class VCMEncodedFrameCallback : public EncodedImageCallback
{
public:
    VCMEncodedFrameCallback();
    virtual ~VCMEncodedFrameCallback();

    


    WebRtc_Word32 Encoded(
        EncodedImage& encodedImage,
        const CodecSpecificInfo* codecSpecificInfo = NULL,
        const RTPFragmentationHeader* fragmentationHeader = NULL);
    


    WebRtc_UWord32 EncodedBytes();
    


    WebRtc_Word32 SetTransportCallback(VCMPacketizationCallback* transport);
    


    void SetMediaOpt (VCMMediaOptimization* mediaOpt);

    void SetPayloadType(WebRtc_UWord8 payloadType) { _payloadType = payloadType; };
    void SetCodecType(VideoCodecType codecType) {_codecType = codecType;};
    void SetInternalSource(bool internalSource) { _internalSource = internalSource; };

private:
    



    static void CopyCodecSpecific(const CodecSpecificInfo& info,
                                  RTPVideoHeader** rtp);

    VCMPacketizationCallback* _sendCallback;
    VCMMediaOptimization*     _mediaOpt;
    WebRtc_UWord32            _encodedBytes;
    WebRtc_UWord8             _payloadType;
    VideoCodecType            _codecType;
    bool                      _internalSource;
#ifdef DEBUG_ENCODER_BIT_STREAM
    FILE*                     _bitStreamAfterEncoder;
#endif
};





class VCMGenericEncoder
{
    friend class VCMCodecDataBase;
public:
    VCMGenericEncoder(VideoEncoder& encoder, bool internalSource = false);
    ~VCMGenericEncoder();
    


    WebRtc_Word32 Release();
    


    WebRtc_Word32 InitEncode(const VideoCodec* settings,
                             WebRtc_Word32 numberOfCores,
                             WebRtc_UWord32 maxPayloadSize);
    






    WebRtc_Word32 Encode(const I420VideoFrame& inputFrame,
                         const CodecSpecificInfo* codecSpecificInfo,
                         const std::vector<FrameType>& frameTypes);
    



    WebRtc_Word32 SetRates(WebRtc_UWord32 newBitRate, WebRtc_UWord32 frameRate);
    


    WebRtc_Word32 SetChannelParameters(WebRtc_Word32 packetLoss, int rtt);
    WebRtc_Word32 CodecConfigParameters(WebRtc_UWord8* buffer, WebRtc_Word32 size);
    


    WebRtc_Word32 RegisterEncodeCallback(VCMEncodedFrameCallback* VCMencodedFrameCallback);
    


    WebRtc_UWord32 BitRate() const;
     


    WebRtc_UWord32 FrameRate() const;

    WebRtc_Word32 SetPeriodicKeyFrames(bool enable);

    WebRtc_Word32 RequestFrame(const std::vector<FrameType>& frame_types);

    bool InternalSource() const;

private:
    VideoEncoder&               _encoder;
    VideoCodecType              _codecType;
    VCMEncodedFrameCallback*    _VCMencodedFrameCallback;
    WebRtc_UWord32              _bitRate;
    WebRtc_UWord32              _frameRate;
    bool                        _internalSource;
}; 

} 

#endif 
