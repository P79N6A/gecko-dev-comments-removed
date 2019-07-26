









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_DEFINES_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_DEFINES_H_

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc
{

#ifndef NULL
    #define NULL    0
#endif

enum {kVideoCaptureUniqueNameLength =1024}; 
enum {kVideoCaptureDeviceNameLength =256}; 
enum {kVideoCaptureProductIdLength =128}; 


enum VideoCaptureRotation
{
    kCameraRotate0 = 0,
    kCameraRotate90 = 5,
    kCameraRotate180 = 10,
    kCameraRotate270 = 15
};

struct VideoCaptureCapability
{
    int32_t width;
    int32_t height;
    int32_t maxFPS;
    int32_t expectedCaptureDelay;
    RawVideoType rawType;
    VideoCodecType codecType;
    bool interlaced;

    VideoCaptureCapability()
    {
        width = 0;
        height = 0;
        maxFPS = 0;
        expectedCaptureDelay = 0;
        rawType = kVideoUnknown;
        codecType = kVideoCodecUnknown;
        interlaced = false;
    }
    ;
    bool operator!=(const VideoCaptureCapability &other) const
    {
        if (width != other.width)
            return true;
        if (height != other.height)
            return true;
        if (maxFPS != other.maxFPS)
            return true;
        if (rawType != other.rawType)
            return true;
        if (codecType != other.codecType)
            return true;
        if (interlaced != other.interlaced)
            return true;
        return false;
    }
    bool operator==(const VideoCaptureCapability &other) const
    {
        return !operator!=(other);
    }
};

enum VideoCaptureAlarm
{
    Raised = 0,
    Cleared = 1
};




class VideoCaptureExternal
{
public:
    
    virtual int32_t IncomingFrame(uint8_t* videoFrame,
                                  int32_t videoFrameLength,
                                  const VideoCaptureCapability& frameInfo,
                                  int64_t captureTime = 0) = 0;
    virtual int32_t IncomingI420VideoFrame(I420VideoFrame* video_frame,
                                           int64_t captureTime = 0) = 0;

protected:
    ~VideoCaptureExternal() {}
};


class VideoCaptureDataCallback
{
public:
    virtual void OnIncomingCapturedFrame(const int32_t id,
                                         I420VideoFrame& videoFrame) = 0;
    virtual void OnCaptureDelayChanged(const int32_t id,
                                       const int32_t delay) = 0;
protected:
    virtual ~VideoCaptureDataCallback(){}
};

class VideoCaptureFeedBack
{
public:
    virtual void OnCaptureFrameRate(const int32_t id,
                                    const uint32_t frameRate) = 0;
    virtual void OnNoPictureAlarm(const int32_t id,
                                  const VideoCaptureAlarm alarm) = 0;
protected:
    virtual ~VideoCaptureFeedBack(){}
};

}  

#endif  
