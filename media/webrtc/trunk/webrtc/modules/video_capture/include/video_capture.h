









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_H_

#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/video_capture/include/video_capture_defines.h"

#if defined(ANDROID) && !defined(WEBRTC_GONK)
#include <jni.h>
#endif

namespace webrtc {

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD) && !defined(WEBRTC_GONK)
int32_t SetCaptureAndroidVM(JavaVM* javaVM);
#endif

class VideoCaptureModule: public RefCountedModule {
 public:
  
  class DeviceInfo {
   public:
    virtual uint32_t NumberOfDevices() = 0;

    
    
    
    
    
    
    
    virtual int32_t GetDeviceName(
        uint32_t deviceNumber,
        char* deviceNameUTF8,
        uint32_t deviceNameLength,
        char* deviceUniqueIdUTF8,
        uint32_t deviceUniqueIdUTF8Length,
        char* productUniqueIdUTF8 = 0,
        uint32_t productUniqueIdUTF8Length = 0) = 0;


    
    virtual int32_t NumberOfCapabilities(
        const char* deviceUniqueIdUTF8) = 0;

    
    virtual int32_t GetCapability(
        const char* deviceUniqueIdUTF8,
        const uint32_t deviceCapabilityNumber,
        VideoCaptureCapability& capability) = 0;

    
    
    virtual int32_t GetOrientation(
        const char* deviceUniqueIdUTF8,
        VideoCaptureRotation& orientation) = 0;

    
    
    
    virtual int32_t GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8,
        const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting) = 0;

     
    virtual int32_t DisplayCaptureSettingsDialogBox(
        const char* deviceUniqueIdUTF8,
        const char* dialogTitleUTF8,
        void* parentWindow,
        uint32_t positionX,
        uint32_t positionY) = 0;

    virtual ~DeviceInfo() {}
  };

  class VideoCaptureEncodeInterface {
   public:
    virtual int32_t ConfigureEncoder(const VideoCodec& codec,
                                     uint32_t maxPayloadSize) = 0;
    
    
    
    virtual int32_t SetRates(int32_t newBitRate, int32_t frameRate) = 0;
    
    
    
    
    virtual int32_t SetChannelParameters(uint32_t packetLoss, int rtt) = 0;

    
    virtual int32_t EncodeFrameType(const FrameType type) = 0;
  protected:
    virtual ~VideoCaptureEncodeInterface() {
    }
  };

  
  virtual void RegisterCaptureDataCallback(
      VideoCaptureDataCallback& dataCallback) = 0;

  
  virtual void DeRegisterCaptureDataCallback() = 0;

  
  virtual void RegisterCaptureCallback(VideoCaptureFeedBack& callBack) = 0;

  
  virtual void DeRegisterCaptureCallback() = 0;

  
  virtual int32_t StartCapture(
      const VideoCaptureCapability& capability) = 0;

  virtual int32_t StopCapture() = 0;

  
  virtual const char* CurrentDeviceName() const = 0;

  
  virtual bool CaptureStarted() = 0;

  
  virtual int32_t CaptureSettings(VideoCaptureCapability& settings) = 0;

  virtual void SetCaptureDelay(int32_t delayMS) = 0;

  
  virtual int32_t CaptureDelay() = 0;

  
  
  
  
  virtual int32_t SetCaptureRotation(VideoCaptureRotation rotation) = 0;

  
  
  virtual VideoCaptureEncodeInterface* GetEncodeInterface(
      const VideoCodec& codec) = 0;

  virtual void EnableFrameRateCallback(const bool enable) = 0;
  virtual void EnableNoPictureAlarm(const bool enable) = 0;

protected:
  virtual ~VideoCaptureModule() {};
};

}  
#endif  
