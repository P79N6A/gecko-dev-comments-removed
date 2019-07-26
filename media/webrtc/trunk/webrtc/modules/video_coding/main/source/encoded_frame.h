









#ifndef WEBRTC_MODULES_VIDEO_CODING_ENCODED_FRAME_H_
#define WEBRTC_MODULES_VIDEO_CODING_ENCODED_FRAME_H_

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/common_video/interface/video_image.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"

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
    


    void SetRenderTime(const int64_t renderTimeMs) {_renderTimeMs = renderTimeMs;}

    


    void SetEncodedSize(uint32_t width, uint32_t height)
                       { _encodedWidth  = width; _encodedHeight = height; }
    


    const webrtc::EncodedImage& EncodedImage() const
                       { return static_cast<const webrtc::EncodedImage&>(*this); }
    


    const uint8_t* Buffer() const {return _buffer;}
    


    uint32_t Length() const {return _length;}
    


    uint32_t TimeStamp() const {return _timeStamp;}
    


    int64_t RenderTimeMs() const {return _renderTimeMs;}
    


    webrtc::FrameType FrameType() const {return ConvertFrameType(_frameType);}
    


    bool Complete() const { return _completeFrame; }
    


    bool MissingFrame() const { return _missingFrame; }
    


    uint8_t PayloadType() const { return _payloadType; }
    





    const CodecSpecificInfo* CodecSpecific() const {return &_codecSpecificInfo;}

    const RTPFragmentationHeader* FragmentationHeader() const;

    static webrtc::FrameType ConvertFrameType(VideoFrameType frameType);
    static VideoFrameType ConvertFrameType(webrtc::FrameType frameType);
    static void ConvertFrameTypes(
        const std::vector<webrtc::FrameType>& frame_types,
        std::vector<VideoFrameType>* video_frame_types);

protected:
    





    int32_t VerifyAndAllocate(const uint32_t minimumSize);

    void Reset();

    void CopyCodecSpecific(const RTPVideoHeader* header);

    int64_t                 _renderTimeMs;
    uint8_t                 _payloadType;
    bool                          _missingFrame;
    CodecSpecificInfo             _codecSpecificInfo;
    webrtc::VideoCodecType        _codec;
    RTPFragmentationHeader        _fragmentation;
};

}  

#endif 
