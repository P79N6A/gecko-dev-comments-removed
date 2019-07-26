









#ifndef WEBRTC_MODULES_VIDEO_CODING_ENCODED_FRAME_H_
#define WEBRTC_MODULES_VIDEO_CODING_ENCODED_FRAME_H_

#include <vector>

#include "common_types.h"
#include "common_video/interface/video_image.h"
#include "modules/interface/module_common_types.h"
#include "modules/video_coding/codecs/interface/video_codec_interface.h"
#include "modules/video_coding/main/interface/video_coding_defines.h"

namespace webrtc
{

class VCMEncodedFrame : protected EncodedImage
{
public:
    VCMEncodedFrame();
    VCMEncodedFrame(const webrtc::EncodedImage& rhs);
    VCMEncodedFrame(const VCMEncodedFrame& rhs);

    ~VCMEncodedFrame();
    


    void Free();
    


    void SetRenderTime(const WebRtc_Word64 renderTimeMs) {_renderTimeMs = renderTimeMs;}

    


    void SetEncodedSize(WebRtc_UWord32 width, WebRtc_UWord32 height)
                       { _encodedWidth  = width; _encodedHeight = height; }
    


    const webrtc::EncodedImage& EncodedImage() const
                       { return static_cast<const webrtc::EncodedImage&>(*this); }
    


    const WebRtc_UWord8* Buffer() const {return _buffer;}
    


    WebRtc_UWord32 Length() const {return _length;}
    


    WebRtc_UWord32 TimeStamp() const {return _timeStamp;}
    


    WebRtc_Word64 RenderTimeMs() const {return _renderTimeMs;}
    


    webrtc::FrameType FrameType() const {return ConvertFrameType(_frameType);}
    


    bool Complete() const { return _completeFrame; }
    


    bool MissingFrame() const { return _missingFrame; }
    


    WebRtc_UWord8 PayloadType() const { return _payloadType; }
    





    const CodecSpecificInfo* CodecSpecific() const {return &_codecSpecificInfo;}

    const RTPFragmentationHeader* FragmentationHeader() const;

    WebRtc_Word32 Store(VCMFrameStorageCallback& storeCallback) const;

    static webrtc::FrameType ConvertFrameType(VideoFrameType frameType);
    static VideoFrameType ConvertFrameType(webrtc::FrameType frameType);
    static void ConvertFrameTypes(
        const std::vector<webrtc::FrameType>& frame_types,
        std::vector<VideoFrameType>* video_frame_types);

protected:
    





    WebRtc_Word32 VerifyAndAllocate(const WebRtc_UWord32 minimumSize);

    void Reset();

    void CopyCodecSpecific(const RTPVideoHeader* header);

    WebRtc_Word64                 _renderTimeMs;
    WebRtc_UWord8                 _payloadType;
    bool                          _missingFrame;
    CodecSpecificInfo             _codecSpecificInfo;
    webrtc::VideoCodecType        _codec;
    RTPFragmentationHeader        _fragmentation;
};

} 

#endif 
