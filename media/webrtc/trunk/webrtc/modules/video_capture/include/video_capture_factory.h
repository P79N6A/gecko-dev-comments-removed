












#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_FACTORY_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_VIDEO_CAPTURE_FACTORY_H_

#include "webrtc/modules/video_capture/include/video_capture.h"

namespace webrtc {

class VideoCaptureFactory {
 public:
  
  
  
  
  static VideoCaptureModule* Create(const int32_t id,
                                    const char* deviceUniqueIdUTF8);

  
  
  
  static VideoCaptureModule* Create(const int32_t id,
                                    VideoCaptureExternal*& externalCapture);

  static VideoCaptureModule::DeviceInfo* CreateDeviceInfo(
      const int32_t id);

#ifdef WEBRTC_ANDROID
  static int32_t SetAndroidObjects(void* javaVM, void* javaContext);
#endif

 private:
  ~VideoCaptureFactory();
};

}  

#endif  
