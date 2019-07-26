









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_CODER_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_CODER_H_

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include "webrtc/engine_configurations.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"

namespace webrtc {
class VideoCoder : public VCMPacketizationCallback, public VCMReceiveCallback
{
public:
    VideoCoder(uint32_t instanceID);
    ~VideoCoder();

    int32_t SetEncodeCodec(VideoCodec& videoCodecInst,
                           uint32_t numberOfCores,
                           uint32_t maxPayloadSize);


    
    
    int32_t SetDecodeCodec(VideoCodec& videoCodecInst, int32_t numberOfCores);

    int32_t Decode(I420VideoFrame& decodedVideo,
                   const EncodedVideoData& encodedData);

    int32_t Encode(const I420VideoFrame& videoFrame,
                   EncodedVideoData& videoEncodedData);

    int8_t DefaultPayloadType(const char* plName);

private:
    
    
    int32_t FrameToRender(I420VideoFrame& videoFrame);

    
    
    int32_t SendData(
        FrameType ,
        uint8_t ,
        uint32_t ,
        int64_t capture_time_ms,
        const uint8_t* payloadData,
        uint32_t payloadSize,
        const RTPFragmentationHeader& ,
        const RTPVideoHeader* rtpTypeHdr);

    VideoCodingModule* _vcm;
    I420VideoFrame* _decodedVideo;
    EncodedVideoData* _videoEncodedData;
};
}  
#endif 
#endif 
