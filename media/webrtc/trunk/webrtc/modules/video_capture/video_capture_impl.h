









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_IMPL_H_





#include "video_capture.h"
#include "video_capture_config.h"
#include "tick_util.h"
#include "common_video/interface/i420_video_frame.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"

namespace webrtc
{
class CriticalSectionWrapper;

namespace videocapturemodule {

class VideoCaptureImpl: public VideoCaptureModule, public VideoCaptureExternal
{
public:

    





    static VideoCaptureModule* Create(const WebRtc_Word32 id,
                                      const char* deviceUniqueIdUTF8);

    





    static VideoCaptureModule* Create(const WebRtc_Word32 id,
                                      VideoCaptureExternal*& externalCapture);

    static DeviceInfo* CreateDeviceInfo(const WebRtc_Word32 id);

    
    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

    
    virtual WebRtc_Word32 RegisterCaptureDataCallback(VideoCaptureDataCallback& dataCallback);
    virtual WebRtc_Word32 DeRegisterCaptureDataCallback();
    virtual WebRtc_Word32 RegisterCaptureCallback(VideoCaptureFeedBack& callBack);
    virtual WebRtc_Word32 DeRegisterCaptureCallback();

    virtual WebRtc_Word32 SetCaptureDelay(WebRtc_Word32 delayMS);
    virtual WebRtc_Word32 CaptureDelay();
    virtual WebRtc_Word32 SetCaptureRotation(VideoCaptureRotation rotation);

    virtual WebRtc_Word32 EnableFrameRateCallback(const bool enable);
    virtual WebRtc_Word32 EnableNoPictureAlarm(const bool enable);

    virtual const char* CurrentDeviceName() const;

    
    virtual WebRtc_Word32 TimeUntilNextProcess();
    virtual WebRtc_Word32 Process();

    
    virtual WebRtc_Word32 IncomingFrame(WebRtc_UWord8* videoFrame,
                                        WebRtc_Word32 videoFrameLength,
                                        const VideoCaptureCapability& frameInfo,
                                        WebRtc_Word64 captureTime = 0);
    virtual WebRtc_Word32 IncomingFrameI420(
        const VideoFrameI420& video_frame,
        WebRtc_Word64 captureTime = 0);

    
    virtual WebRtc_Word32 StartCapture(const VideoCaptureCapability& capability)
    {
        _requestedCapability = capability;
        return -1;
    }
    virtual WebRtc_Word32 StopCapture()   { return -1; }
    virtual bool CaptureStarted() {return false; }
    virtual WebRtc_Word32 CaptureSettings(VideoCaptureCapability& )
    { return -1; }
    VideoCaptureEncodeInterface* GetEncodeInterface(const VideoCodec& )
    { return NULL; }

protected:
    VideoCaptureImpl(const WebRtc_Word32 id);
    virtual ~VideoCaptureImpl();
    WebRtc_Word32 DeliverCapturedFrame(I420VideoFrame& captureFrame,
                                       WebRtc_Word64 capture_time);
    WebRtc_Word32 DeliverEncodedCapturedFrame(VideoFrame& captureFrame,
                                              WebRtc_Word64 capture_time,
                                              VideoCodecType codec_type);

    WebRtc_Word32 _id; 
    char* _deviceUniqueId; 
    CriticalSectionWrapper& _apiCs;
    WebRtc_Word32 _captureDelay; 
    VideoCaptureCapability _requestedCapability; 
private:
    void UpdateFrameCount();
    WebRtc_UWord32 CalculateFrameRate(const TickTime& now);

    CriticalSectionWrapper& _callBackCs;

    TickTime _lastProcessTime; 
    TickTime _lastFrameRateCallbackTime; 
    bool _frameRateCallBack; 
    bool _noPictureAlarmCallBack; 
    VideoCaptureAlarm _captureAlarm; 

    WebRtc_Word32 _setCaptureDelay; 
    VideoCaptureDataCallback* _dataCallBack;
    VideoCaptureFeedBack* _captureCallBack;

    TickTime _lastProcessFrameCount;
    TickTime _incomingFrameTimes[kFrameRateCountHistorySize];
    VideoRotationMode _rotateFrame; 

    I420VideoFrame _captureFrame;
    VideoFrame _capture_encoded_frame;

    
    WebRtc_Word64 last_capture_time_;
};
} 
} 
#endif  
