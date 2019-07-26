









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_DEFINES_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_DEFINES_H_

#include "webrtc/typedefs.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/common_video/interface/i420_video_frame.h"

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
    WebRtc_Word32 width;
    WebRtc_Word32 height;
    WebRtc_Word32 maxFPS;
    WebRtc_Word32 expectedCaptureDelay;
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



struct VideoFrameI420
{
  VideoFrameI420() {
    y_plane = NULL;
    u_plane = NULL;
    v_plane = NULL;
    y_pitch = 0;
    u_pitch = 0;
    v_pitch = 0;
    width = 0;
    height = 0;
  }

  unsigned char* y_plane;
  unsigned char* u_plane;
  unsigned char* v_plane;

  int y_pitch;
  int u_pitch;
  int v_pitch;

  unsigned short width;
  unsigned short height;
};




class VideoCaptureExternal
{
public:
    virtual WebRtc_Word32 IncomingFrame(WebRtc_UWord8* videoFrame,
                                        WebRtc_Word32 videoFrameLength,
                                        const VideoCaptureCapability& frameInfo,
                                        WebRtc_Word64 captureTime = 0) = 0;
    virtual WebRtc_Word32 IncomingFrameI420(const VideoFrameI420& video_frame,
                                            WebRtc_Word64 captureTime = 0) = 0;
protected:
    ~VideoCaptureExternal() {}
};


class VideoCaptureDataCallback
{
public:
    virtual void OnIncomingCapturedFrame(const WebRtc_Word32 id,
                                         I420VideoFrame& videoFrame) = 0;
    virtual void OnIncomingCapturedEncodedFrame(const WebRtc_Word32 id,
                                                VideoFrame& videoFrame,
                                                VideoCodecType codecType) = 0;
    virtual void OnCaptureDelayChanged(const WebRtc_Word32 id,
                                       const WebRtc_Word32 delay) = 0;
protected:
    virtual ~VideoCaptureDataCallback(){}
};

class VideoCaptureFeedBack
{
public:
    virtual void OnCaptureFrameRate(const WebRtc_Word32 id,
                                    const WebRtc_UWord32 frameRate) = 0;
    virtual void OnNoPictureAlarm(const WebRtc_Word32 id,
                                  const VideoCaptureAlarm alarm) = 0;
protected:
    virtual ~VideoCaptureFeedBack(){}
};

}  

#endif  
