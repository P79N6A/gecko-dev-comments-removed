









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_CODER_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_CODER_H_

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include "engine_configurations.h"
#include "video_coding.h"

namespace webrtc {
class VideoCoder : public VCMPacketizationCallback, public VCMReceiveCallback
{
public:
    VideoCoder(WebRtc_UWord32 instanceID);
    ~VideoCoder();

    WebRtc_Word32 ResetDecoder();

    WebRtc_Word32 SetEncodeCodec(VideoCodec& videoCodecInst,
                                 WebRtc_UWord32 numberOfCores,
                                 WebRtc_UWord32 maxPayloadSize);


    
    
    WebRtc_Word32 SetDecodeCodec(VideoCodec& videoCodecInst,
                                 WebRtc_Word32 numberOfCores);

    WebRtc_Word32 Decode(VideoFrame& decodedVideo,
                         const EncodedVideoData& encodedData);

    WebRtc_Word32 Encode(const VideoFrame& videoFrame,
                         EncodedVideoData& videoEncodedData);

    WebRtc_Word8 DefaultPayloadType(const char* plName);

private:
    
    
    WebRtc_Word32 FrameToRender(VideoFrame& videoFrame);

    
    
    WebRtc_Word32 SendData(
        const FrameType ,
        const WebRtc_UWord8 ,
        const WebRtc_UWord32 ,
        const WebRtc_UWord8* payloadData,
        const WebRtc_UWord32 payloadSize,
        const RTPFragmentationHeader& ,
        const RTPVideoHeader* rtpTypeHdr);

    WebRtc_UWord32 _instanceID;
    VideoCodingModule* _vcm;
    VideoFrame* _decodedVideo;
    EncodedVideoData* _videoEncodedData;
};
} 
#endif 
#endif 
