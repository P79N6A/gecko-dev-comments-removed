









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_ANDROID_DEVICE_INFO_ANDROID_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_ANDROID_DEVICE_INFO_ANDROID_H_

#include <jni.h>
#include "../video_capture_impl.h"
#include "../device_info_impl.h"

#define AndroidJavaCaptureDeviceInfoClass "org/webrtc/videoengine/VideoCaptureDeviceInfoAndroid"
#define AndroidJavaCaptureCapabilityClass "org/webrtc/videoengine/CaptureCapabilityAndroid"

namespace webrtc
{
namespace videocapturemodule
{







class DeviceInfoAndroid : public DeviceInfoImpl {

 public:
  DeviceInfoAndroid(const WebRtc_Word32 id);
  WebRtc_Word32 Init();
  virtual ~DeviceInfoAndroid();
  virtual WebRtc_UWord32 NumberOfDevices();
  virtual WebRtc_Word32 GetDeviceName(
      WebRtc_UWord32 deviceNumber,
      char* deviceNameUTF8,
      WebRtc_UWord32 deviceNameLength,
      char* deviceUniqueIdUTF8,
      WebRtc_UWord32 deviceUniqueIdUTF8Length,
      char* productUniqueIdUTF8 = 0,
      WebRtc_UWord32 productUniqueIdUTF8Length = 0);
  virtual WebRtc_Word32 CreateCapabilityMap(const char* deviceUniqueIdUTF8);

  virtual WebRtc_Word32 DisplayCaptureSettingsDialogBox(
      const char* ,
      const char* ,
      void* ,
      WebRtc_UWord32 ,
      WebRtc_UWord32 ) { return -1; }
  virtual WebRtc_Word32 GetOrientation(const char* deviceUniqueIdUTF8,
                                       VideoCaptureRotation& orientation);
 private:
  bool IsDeviceNameMatches(const char* name, const char* deviceUniqueIdUTF8);
  enum {_expectedCaptureDelay = 190};
};

}  
}  

#endif 
