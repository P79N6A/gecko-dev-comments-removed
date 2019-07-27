









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_MAIN_SOURCE_DESKTOP_CAPTURE_IMPL_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_MAIN_SOURCE_DESKTOP_CAPTURE_IMPL_H_





#include <string>

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/desktop_capture/screen_capturer.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/modules/video_capture/video_capture_config.h"
#include "webrtc/modules/desktop_capture/shared_memory.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/modules/desktop_capture/mouse_cursor_shape.h"
#include "webrtc/modules/desktop_capture/desktop_device_info.h"
#include "webrtc/modules/desktop_capture/desktop_and_cursor_composer.h"
#include "webrtc/video_engine/include/vie_capture.h"

using namespace webrtc::videocapturemodule;

namespace webrtc {

class CriticalSectionWrapper;
class VideoCaptureEncodeInterface;




class ScreenDeviceInfoImpl : public VideoCaptureModule::DeviceInfo {
public:
  ScreenDeviceInfoImpl(const int32_t id);
  virtual ~ScreenDeviceInfoImpl(void);

  int32_t Init();
  int32_t Refresh();

  virtual uint32_t NumberOfDevices();
  virtual int32_t GetDeviceName(uint32_t deviceNumber,
                                char* deviceNameUTF8,
                                uint32_t deviceNameLength,
                                char* deviceUniqueIdUTF8,
                                uint32_t deviceUniqueIdUTF8Length,
                                char* productUniqueIdUTF8,
                                uint32_t productUniqueIdUTF8Length);

  virtual int32_t DisplayCaptureSettingsDialogBox(const char* deviceUniqueIdUTF8,
                                                  const char* dialogTitleUTF8,
                                                  void* parentWindow,
                                                  uint32_t positionX,
                                                  uint32_t positionY);
  virtual int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8);
  virtual int32_t GetCapability(const char* deviceUniqueIdUTF8,
                                const uint32_t deviceCapabilityNumber,
                                VideoCaptureCapability& capability);

  virtual int32_t GetBestMatchedCapability(const char* deviceUniqueIdUTF8,
                                           const VideoCaptureCapability& requested,
                                           VideoCaptureCapability& resulting);
  virtual int32_t GetOrientation(const char* deviceUniqueIdUTF8,
                                 VideoCaptureRotation& orientation);
protected:
  int32_t _id;
  scoped_ptr<DesktopDeviceInfo> desktop_device_info_;

};

class AppDeviceInfoImpl : public VideoCaptureModule::DeviceInfo {
public:
  AppDeviceInfoImpl(const int32_t id);
  virtual ~AppDeviceInfoImpl(void);

  int32_t Init();
  int32_t Refresh();

  virtual uint32_t NumberOfDevices();
  virtual int32_t GetDeviceName(uint32_t deviceNumber,
                                char* deviceNameUTF8,
                                uint32_t deviceNameLength,
                                char* deviceUniqueIdUTF8,
                                uint32_t deviceUniqueIdUTF8Length,
                                char* productUniqueIdUTF8,
                                uint32_t productUniqueIdUTF8Length);

  virtual int32_t DisplayCaptureSettingsDialogBox(const char* deviceUniqueIdUTF8,
                                                  const char* dialogTitleUTF8,
                                                  void* parentWindow,
                                                  uint32_t positionX,
                                                  uint32_t positionY);
  virtual int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8);
  virtual int32_t GetCapability(const char* deviceUniqueIdUTF8,
                                const uint32_t deviceCapabilityNumber,
                                VideoCaptureCapability& capability);

  virtual int32_t GetBestMatchedCapability(const char* deviceUniqueIdUTF8,
                                           const VideoCaptureCapability& requested,
                                           VideoCaptureCapability& resulting);
  virtual int32_t GetOrientation(const char* deviceUniqueIdUTF8,
                                 VideoCaptureRotation& orientation);
protected:
  int32_t _id;
  scoped_ptr<DesktopDeviceInfo> desktop_device_info_;
};

class WindowDeviceInfoImpl : public VideoCaptureModule::DeviceInfo {
public:
  WindowDeviceInfoImpl(const int32_t id) : _id(id) {};
  virtual ~WindowDeviceInfoImpl(void) {};

  int32_t Init();
  int32_t Refresh();

  virtual uint32_t NumberOfDevices();
  virtual int32_t GetDeviceName(uint32_t deviceNumber,
                                char* deviceNameUTF8,
                                uint32_t deviceNameLength,
                                char* deviceUniqueIdUTF8,
                                uint32_t deviceUniqueIdUTF8Length,
                                char* productUniqueIdUTF8,
                                uint32_t productUniqueIdUTF8Length);

  virtual int32_t DisplayCaptureSettingsDialogBox(const char* deviceUniqueIdUTF8,
                                                  const char* dialogTitleUTF8,
                                                  void* parentWindow,
                                                  uint32_t positionX,
                                                  uint32_t positionY);
  virtual int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8);
  virtual int32_t GetCapability(const char* deviceUniqueIdUTF8,
                                const uint32_t deviceCapabilityNumber,
                                VideoCaptureCapability& capability);

  virtual int32_t GetBestMatchedCapability(const char* deviceUniqueIdUTF8,
                                           const VideoCaptureCapability& requested,
                                           VideoCaptureCapability& resulting);
  virtual int32_t GetOrientation(const char* deviceUniqueIdUTF8,
                                 VideoCaptureRotation& orientation);
protected:
  int32_t _id;
  scoped_ptr<DesktopDeviceInfo> desktop_device_info_;

};




class DesktopCaptureImpl: public VideoCaptureModule,
                          public VideoCaptureExternal,
                          public ScreenCapturer::Callback,
                          public ScreenCapturer::MouseShapeObserver
{
public:
  

  static VideoCaptureModule* Create(const int32_t id, const char* uniqueId, const CaptureDeviceType type);
  static VideoCaptureModule::DeviceInfo* CreateDeviceInfo(const int32_t id, const CaptureDeviceType type);

  int32_t Init(const char* uniqueId, const CaptureDeviceType type);
  
  virtual int32_t ChangeUniqueId(const int32_t id) OVERRIDE;

  
  virtual void RegisterCaptureDataCallback(VideoCaptureDataCallback& dataCallback) OVERRIDE;
  virtual void DeRegisterCaptureDataCallback() OVERRIDE;
  virtual void RegisterCaptureCallback(VideoCaptureFeedBack& callBack) OVERRIDE;
  virtual void DeRegisterCaptureCallback() OVERRIDE;

  virtual void SetCaptureDelay(int32_t delayMS) OVERRIDE;
  virtual int32_t CaptureDelay() OVERRIDE;
  virtual int32_t SetCaptureRotation(VideoCaptureRotation rotation) OVERRIDE;

  virtual void EnableFrameRateCallback(const bool enable) OVERRIDE;
  virtual void EnableNoPictureAlarm(const bool enable) OVERRIDE;

  virtual const char* CurrentDeviceName() const OVERRIDE;

  
  virtual int32_t TimeUntilNextProcess() OVERRIDE;
  virtual int32_t Process() OVERRIDE;

  
  
  virtual int32_t IncomingFrame(uint8_t* videoFrame,
                                int32_t videoFrameLength,
                                const VideoCaptureCapability& frameInfo,
                                int64_t captureTime = 0) OVERRIDE;
  virtual int32_t IncomingI420VideoFrame(
                                         I420VideoFrame* video_frame,
                                         int64_t captureTime = 0) OVERRIDE;

  
  virtual int32_t StartCapture(const VideoCaptureCapability& capability) OVERRIDE;
  virtual int32_t StopCapture() OVERRIDE;
  virtual bool CaptureStarted() OVERRIDE;
  virtual int32_t CaptureSettings(VideoCaptureCapability& settings) OVERRIDE;
  VideoCaptureEncodeInterface* GetEncodeInterface(const VideoCodec& codec) OVERRIDE { return NULL; }

  
  virtual SharedMemory* CreateSharedMemory(size_t size) OVERRIDE;
  virtual void OnCaptureCompleted(DesktopFrame* frame) OVERRIDE;

  
  virtual void OnCursorShapeChanged(MouseCursorShape* cursor_shape) OVERRIDE;

protected:
  DesktopCaptureImpl(const int32_t id);
  virtual ~DesktopCaptureImpl();
  int32_t DeliverCapturedFrame(I420VideoFrame& captureFrame,
                               int64_t capture_time);

  static const uint32_t kMaxDesktopCaptureCpuUsage = 50; 

  int32_t _id; 
  std::string _deviceUniqueId; 
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

public:
  static bool Run(void*obj) {
    static_cast<DesktopCaptureImpl*>(obj)->process();
    return true;
  };
  void process();

private:
  scoped_ptr<DesktopAndCursorComposer> desktop_capturer_cursor_composer_;
  EventWrapper& time_event_;
  ThreadWrapper&  capturer_thread_;
};

}  

#endif  
