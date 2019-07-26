









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_H_

#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/video_capture/include/video_capture_defines.h"

namespace webrtc {

#if defined(WEBRTC_ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
WebRtc_Word32 SetCaptureAndroidVM(void* javaVM, void* javaContext);
#endif

class VideoCaptureModule: public RefCountedModule {
 public:
  
  class DeviceInfo {
   public:
    virtual WebRtc_UWord32 NumberOfDevices() = 0;

    
    
    
    
    
    
    
    virtual WebRtc_Word32 GetDeviceName(
        WebRtc_UWord32 deviceNumber,
        char* deviceNameUTF8,
        WebRtc_UWord32 deviceNameLength,
        char* deviceUniqueIdUTF8,
        WebRtc_UWord32 deviceUniqueIdUTF8Length,
        char* productUniqueIdUTF8 = 0,
        WebRtc_UWord32 productUniqueIdUTF8Length = 0) = 0;


    
    virtual WebRtc_Word32 NumberOfCapabilities(
        const char* deviceUniqueIdUTF8) = 0;

    
    virtual WebRtc_Word32 GetCapability(
        const char* deviceUniqueIdUTF8,
        const WebRtc_UWord32 deviceCapabilityNumber,
        VideoCaptureCapability& capability) = 0;

    
    
    virtual WebRtc_Word32 GetOrientation(
        const char* deviceUniqueIdUTF8,
        VideoCaptureRotation& orientation) = 0;

    
    
    
    virtual WebRtc_Word32 GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8,
        const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting) = 0;

     
    virtual WebRtc_Word32 DisplayCaptureSettingsDialogBox(
        const char* deviceUniqueIdUTF8,
        const char* dialogTitleUTF8,
        void* parentWindow,
        WebRtc_UWord32 positionX,
        WebRtc_UWord32 positionY) = 0;

    virtual ~DeviceInfo() {}
  };

  class VideoCaptureEncodeInterface {
   public:
    virtual WebRtc_Word32 ConfigureEncoder(const VideoCodec& codec,
                                           WebRtc_UWord32 maxPayloadSize) = 0;
    
    
    
    virtual WebRtc_Word32 SetRates(WebRtc_Word32 newBitRate,
                                   WebRtc_Word32 frameRate) = 0;
    
    
    
    
    virtual WebRtc_Word32 SetChannelParameters(WebRtc_UWord32 packetLoss,
                                               int rtt) = 0;

    
    virtual WebRtc_Word32 EncodeFrameType(const FrameType type) = 0;
  protected:
    virtual ~VideoCaptureEncodeInterface() {
    }
  };

  
  virtual WebRtc_Word32 RegisterCaptureDataCallback(
      VideoCaptureDataCallback& dataCallback) = 0;

  
  virtual WebRtc_Word32 DeRegisterCaptureDataCallback() = 0;

  
  virtual WebRtc_Word32 RegisterCaptureCallback(
      VideoCaptureFeedBack& callBack) = 0;

  
  virtual WebRtc_Word32 DeRegisterCaptureCallback() = 0;

  
  virtual WebRtc_Word32 StartCapture(
      const VideoCaptureCapability& capability) = 0;

  virtual WebRtc_Word32 StopCapture() = 0;

  
  virtual const char* CurrentDeviceName() const = 0;

  
  virtual bool CaptureStarted() = 0;

  
  virtual WebRtc_Word32 CaptureSettings(VideoCaptureCapability& settings) = 0;

  virtual WebRtc_Word32 SetCaptureDelay(WebRtc_Word32 delayMS) = 0;

  
  virtual WebRtc_Word32 CaptureDelay() = 0;

  
  
  
  
  virtual WebRtc_Word32 SetCaptureRotation(VideoCaptureRotation rotation) = 0;

  
  
  virtual VideoCaptureEncodeInterface* GetEncodeInterface(
      const VideoCodec& codec) = 0;

  virtual WebRtc_Word32 EnableFrameRateCallback(const bool enable) = 0;
  virtual WebRtc_Word32 EnableNoPictureAlarm(const bool enable) = 0;

protected:
  virtual ~VideoCaptureModule() {};
};

} 
#endif  
