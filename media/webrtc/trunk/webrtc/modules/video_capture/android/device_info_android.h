









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_ANDROID_DEVICE_INFO_ANDROID_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_ANDROID_DEVICE_INFO_ANDROID_H_

#include <jni.h>

#include "webrtc/modules/video_capture/device_info_impl.h"
#include "webrtc/modules/video_capture/video_capture_impl.h"

namespace webrtc
{
namespace videocapturemodule
{







class DeviceInfoAndroid : public DeviceInfoImpl {

 public:
  static void SetAndroidCaptureClasses(jclass capabilityClass);
  DeviceInfoAndroid(const int32_t id);
  int32_t Init();
  virtual ~DeviceInfoAndroid();
  virtual uint32_t NumberOfDevices();
  virtual int32_t GetDeviceName(
      uint32_t deviceNumber,
      char* deviceNameUTF8,
      uint32_t deviceNameLength,
      char* deviceUniqueIdUTF8,
      uint32_t deviceUniqueIdUTF8Length,
      char* productUniqueIdUTF8 = 0,
      uint32_t productUniqueIdUTF8Length = 0);
  virtual int32_t CreateCapabilityMap(const char* deviceUniqueIdUTF8);

  virtual int32_t DisplayCaptureSettingsDialogBox(
      const char* ,
      const char* ,
      void* ,
      uint32_t ,
      uint32_t ) { return -1; }
  virtual int32_t GetOrientation(const char* deviceUniqueIdUTF8,
                                 VideoCaptureRotation& orientation);
 private:
  bool IsDeviceNameMatches(const char* name, const char* deviceUniqueIdUTF8);
  enum {_expectedCaptureDelay = 190};
};

}  
}  

#endif 
