









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_IMPL_H_





#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/video_capture/include/video_capture.h"
#include "webrtc/modules/video_capture/video_capture_config.h"
#include "webrtc/system_wrappers/interface/tick_util.h"

namespace webrtc
{
class CriticalSectionWrapper;

namespace videocapturemodule {

class VideoCaptureImpl: public VideoCaptureModule, public VideoCaptureExternal
{
public:

    





    static VideoCaptureModule* Create(const int32_t id,
                                      const char* deviceUniqueIdUTF8);

    





    static VideoCaptureModule* Create(const int32_t id,
                                      VideoCaptureExternal*& externalCapture);

    static DeviceInfo* CreateDeviceInfo(const int32_t id);

    
    
    static int32_t RotationFromDegrees(int degrees,
                                       VideoCaptureRotation* rotation);
    static int32_t RotationInDegrees(VideoCaptureRotation rotation,
                                     int* degrees);

    
    virtual int32_t ChangeUniqueId(const int32_t id);

    
    virtual void RegisterCaptureDataCallback(
        VideoCaptureDataCallback& dataCallback);
    virtual void DeRegisterCaptureDataCallback();
    virtual void RegisterCaptureCallback(VideoCaptureFeedBack& callBack);
    virtual void DeRegisterCaptureCallback();

    virtual void SetCaptureDelay(int32_t delayMS);
    virtual int32_t CaptureDelay();
    virtual int32_t SetCaptureRotation(VideoCaptureRotation rotation);

    virtual void EnableFrameRateCallback(const bool enable);
    virtual void EnableNoPictureAlarm(const bool enable);

    virtual const char* CurrentDeviceName() const;

    
    virtual int32_t TimeUntilNextProcess();
    virtual int32_t Process();

    
    
    virtual int32_t IncomingFrame(uint8_t* videoFrame,
                                  int32_t videoFrameLength,
                                  const VideoCaptureCapability& frameInfo,
                                  int64_t captureTime = 0);

    virtual int32_t IncomingI420VideoFrame(I420VideoFrame* video_frame,
                                           int64_t captureTime = 0);

    
    virtual int32_t StartCapture(const VideoCaptureCapability& capability)
    {
        _requestedCapability = capability;
        return -1;
    }
    virtual int32_t StopCapture()   { return -1; }
    virtual bool CaptureStarted() {return false; }
    virtual int32_t CaptureSettings(VideoCaptureCapability& )
    { return -1; }
    VideoCaptureEncodeInterface* GetEncodeInterface(const VideoCodec& )
    { return NULL; }

protected:
    VideoCaptureImpl(const int32_t id);
    virtual ~VideoCaptureImpl();
    int32_t DeliverCapturedFrame(I420VideoFrame& captureFrame,
                                 int64_t capture_time);

    int32_t _id; 
    char* _deviceUniqueId; 
    CriticalSectionWrapper& _apiCs;
    int32_t _captureDelay; 
    VideoCaptureCapability _requestedCapability; 
private:
    void UpdateFrameCount();
    uint32_t CalculateFrameRate(const TickTime& now);

    CriticalSectionWrapper& _callBackCs;

    TickTime _lastProcessTime; 
    TickTime _lastFrameRateCallbackTime; 
    bool _frameRateCallBack; 
    bool _noPictureAlarmCallBack; 
    VideoCaptureAlarm _captureAlarm; 

    int32_t _setCaptureDelay; 
    VideoCaptureDataCallback* _dataCallBack;
    VideoCaptureFeedBack* _captureCallBack;

    TickTime _lastProcessFrameCount;
    TickTime _incomingFrameTimes[kFrameRateCountHistorySize];
    VideoRotationMode _rotateFrame; 

    I420VideoFrame _captureFrame;
    VideoFrame _capture_encoded_frame;

    
    int64_t last_capture_time_;

    
    const int64_t delta_ntp_internal_ms_;
};
}  
}  
#endif  
