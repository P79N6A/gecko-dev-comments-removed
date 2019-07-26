









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/common_video/interface/video_image.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/codecs/interface/video_error_codes.h"

#include "webrtc/typedefs.h"

namespace webrtc
{

class RTPFragmentationHeader; 



struct CodecSpecificInfoVP8
{
    bool             hasReceivedSLI;
    uint8_t    pictureIdSLI;
    bool             hasReceivedRPSI;
    uint64_t   pictureIdRPSI;
    int16_t    pictureId;         
    bool             nonReference;
    uint8_t    simulcastIdx;
    uint8_t    temporalIdx;
    bool             layerSync;
    int              tl0PicIdx;         
    int8_t     keyIdx;            
};

struct CodecSpecificInfoGeneric {
  uint8_t simulcast_idx;
};

struct CodecSpecificInfoH264 {
  unsigned char nalu_header;
  bool          single_nalu;
};

union CodecSpecificInfoUnion {
    CodecSpecificInfoGeneric   generic;
    CodecSpecificInfoVP8       VP8;
    CodecSpecificInfoH264      H264;
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

    
    
    
    
    
    
    
    
    
    virtual int32_t
    Encoded(EncodedImage& encodedImage,
            const CodecSpecificInfo* codecSpecificInfo = NULL,
            const RTPFragmentationHeader* fragmentation = NULL) = 0;
};

class VideoEncoder
{
public:
    virtual ~VideoEncoder() {};

    
    
    
    
    
    
    
    
    
    virtual int32_t InitEncode(const VideoCodec* codecSettings, int32_t numberOfCores, uint32_t maxPayloadSize) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual int32_t Encode(
        const I420VideoFrame& inputImage,
        const CodecSpecificInfo* codecSpecificInfo,
        const std::vector<VideoFrameType>* frame_types) = 0;

    
    
    
    
    
    
    virtual int32_t RegisterEncodeCompleteCallback(EncodedImageCallback* callback) = 0;

    
    
    
    virtual int32_t Release() = 0;

    
    
    
    
    
    
    
    
    virtual int32_t SetChannelParameters(uint32_t packetLoss, int rtt) = 0;

    
    
    
    
    
    
    virtual int32_t SetRates(uint32_t newBitRate, uint32_t frameRate) = 0;

    
    
    
    
    
    
    virtual int32_t SetPeriodicKeyFrames(bool enable) { return WEBRTC_VIDEO_CODEC_ERROR; }

    
    
    
    
    
    
    
    virtual int32_t CodecConfigParameters(uint8_t* , int32_t ) { return WEBRTC_VIDEO_CODEC_ERROR; }
};

class DecodedImageCallback
{
public:
    virtual ~DecodedImageCallback() {};

    
    
    
    
    
    
    virtual int32_t Decoded(I420VideoFrame& decodedImage) = 0;

    virtual int32_t ReceivedDecodedReferenceFrame(const uint64_t pictureId) {return -1;}

    virtual int32_t ReceivedDecodedFrame(const uint64_t pictureId) {return -1;}
};

class VideoDecoder
{
public:
    virtual ~VideoDecoder() {};

    
    
    
    
    
    
    
    virtual int32_t InitDecode(const VideoCodec* codecSettings, int32_t numberOfCores) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t
    Decode(const EncodedImage& inputImage,
           bool missingFrames,
           const RTPFragmentationHeader* fragmentation,
           const CodecSpecificInfo* codecSpecificInfo = NULL,
           int64_t renderTimeMs = -1) = 0;

    
    
    
    
    
    
    virtual int32_t RegisterDecodeCompleteCallback(DecodedImageCallback* callback) = 0;

    
    
    
    virtual int32_t Release() = 0;

    
    
    
    virtual int32_t Reset() = 0;

    
    
    
    
    
    
    
    
    virtual int32_t SetCodecConfigParameters(const uint8_t* , int32_t ) { return WEBRTC_VIDEO_CODEC_ERROR; }

    
    
    
    virtual VideoDecoder* Copy() { return NULL; }
};

}  

#endif 
