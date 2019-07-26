









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H

#include <vector>

#include "common_types.h"
#include "common_video/interface/i420_video_frame.h"
#include "modules/interface/module_common_types.h"
#include "modules/video_coding/codecs/interface/video_error_codes.h"
#include "common_video/interface/video_image.h"

#include "typedefs.h"

namespace webrtc
{

class RTPFragmentationHeader; 



struct CodecSpecificInfoVP8
{
    bool             hasReceivedSLI;
    WebRtc_UWord8    pictureIdSLI;
    bool             hasReceivedRPSI;
    WebRtc_UWord64   pictureIdRPSI;
    WebRtc_Word16    pictureId;         
    bool             nonReference;
    WebRtc_UWord8    simulcastIdx;
    WebRtc_UWord8    temporalIdx;
    bool             layerSync;
    int              tl0PicIdx;         
    WebRtc_Word8     keyIdx;            
};

union CodecSpecificInfoUnion
{
    CodecSpecificInfoVP8       VP8;
};




struct CodecSpecificInfo
{
    VideoCodecType   codecType;
    CodecSpecificInfoUnion codecSpecific;
};

class EncodedImageCallback
{
public:
    virtual ~EncodedImageCallback() {};

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32
    Encoded(EncodedImage& encodedImage,
            const CodecSpecificInfo* codecSpecificInfo = NULL,
            const RTPFragmentationHeader* fragmentation = NULL) = 0;
};

class VideoEncoder
{
public:
    virtual ~VideoEncoder() {};

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 InitEncode(const VideoCodec* codecSettings, WebRtc_Word32 numberOfCores, WebRtc_UWord32 maxPayloadSize) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 Encode(
        const I420VideoFrame& inputImage,
        const CodecSpecificInfo* codecSpecificInfo,
        const std::vector<VideoFrameType>* frame_types) = 0;

    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterEncodeCompleteCallback(EncodedImageCallback* callback) = 0;

    
    
    
    virtual WebRtc_Word32 Release() = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetChannelParameters(WebRtc_UWord32 packetLoss,
                                               int rtt) = 0;

    
    
    
    
    
    
    virtual WebRtc_Word32 SetRates(WebRtc_UWord32 newBitRate, WebRtc_UWord32 frameRate) = 0;

    
    
    
    
    
    
    virtual WebRtc_Word32 SetPeriodicKeyFrames(bool enable) { return WEBRTC_VIDEO_CODEC_ERROR; }

    
    
    
    
    
    
    
    virtual WebRtc_Word32 CodecConfigParameters(WebRtc_UWord8* , WebRtc_Word32 ) { return WEBRTC_VIDEO_CODEC_ERROR; }
};

class DecodedImageCallback
{
public:
    virtual ~DecodedImageCallback() {};

    
    
    
    
    
    
    virtual WebRtc_Word32 Decoded(I420VideoFrame& decodedImage) = 0;

    virtual WebRtc_Word32 ReceivedDecodedReferenceFrame(const WebRtc_UWord64 pictureId) {return -1;}

    virtual WebRtc_Word32 ReceivedDecodedFrame(const WebRtc_UWord64 pictureId) {return -1;}
};

class VideoDecoder
{
public:
    virtual ~VideoDecoder() {};

    
    
    
    
    
    
    
    virtual WebRtc_Word32 InitDecode(const VideoCodec* codecSettings, WebRtc_Word32 numberOfCores) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32
    Decode(const EncodedImage& inputImage,
           bool missingFrames,
           const RTPFragmentationHeader* fragmentation,
           const CodecSpecificInfo* codecSpecificInfo = NULL,
           WebRtc_Word64 renderTimeMs = -1) = 0;

    
    
    
    
    
    
    virtual WebRtc_Word32 RegisterDecodeCompleteCallback(DecodedImageCallback* callback) = 0;

    
    
    
    virtual WebRtc_Word32 Release() = 0;

    
    
    
    virtual WebRtc_Word32 Reset() = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetCodecConfigParameters(const WebRtc_UWord8* , WebRtc_Word32 ) { return WEBRTC_VIDEO_CODEC_ERROR; }

    
    
    
    virtual VideoDecoder* Copy() { return NULL; }
};

} 

#endif 
