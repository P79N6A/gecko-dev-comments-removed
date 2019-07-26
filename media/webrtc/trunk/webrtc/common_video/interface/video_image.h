









#ifndef COMMON_VIDEO_INTERFACE_VIDEO_IMAGE_H
#define COMMON_VIDEO_INTERFACE_VIDEO_IMAGE_H

#include <stdlib.h>
#include "webrtc/typedefs.h"

namespace webrtc
{

enum VideoFrameType
{
    kKeyFrame = 0,
    kDeltaFrame = 1,
    kGoldenFrame = 2,
    kAltRefFrame = 3,
    kSkipFrame = 4
};

class EncodedImage
{
public:
    EncodedImage()
        : _encodedWidth(0),
          _encodedHeight(0),
          _timeStamp(0),
          capture_time_ms_(0),
          _frameType(kDeltaFrame),
          _buffer(NULL),
          _length(0),
          _size(0),
          _completeFrame(false) {}

    EncodedImage(uint8_t* buffer,
                 uint32_t length,
                 uint32_t size)
        : _encodedWidth(0),
          _encodedHeight(0),
          _timeStamp(0),
          capture_time_ms_(0),
          _frameType(kDeltaFrame),
          _buffer(buffer),
          _length(length),
          _size(size),
          _completeFrame(false) {}

    uint32_t                    _encodedWidth;
    uint32_t                    _encodedHeight;
    uint32_t                    _timeStamp;
    int64_t                      capture_time_ms_;
    VideoFrameType              _frameType;
    uint8_t*                    _buffer;
    uint32_t                    _length;
    uint32_t                    _size;
    bool                        _completeFrame;
};

}  

#endif 
