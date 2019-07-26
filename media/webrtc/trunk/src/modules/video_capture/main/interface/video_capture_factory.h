












#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_INTERFACE_VIDEO_CAPTURE_FACTORY_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_INTERFACE_VIDEO_CAPTURE_FACTORY_H_

#include "modules/video_capture/main/interface/video_capture.h"

namespace webrtc {

class VideoCaptureFactory {
 public:
  
  
  
  
  static VideoCaptureModule* Create(const WebRtc_Word32 id,
                                    const char* deviceUniqueIdUTF8);

  
  
  
  static VideoCaptureModule* Create(const WebRtc_Word32 id,
                                    VideoCaptureExternal*& externalCapture);

  static VideoCaptureModule::DeviceInfo* CreateDeviceInfo(
      const WebRtc_Word32 id);

#ifdef WEBRTC_ANDROID
  static WebRtc_Word32 SetAndroidObjects(void* javaVM, void* javaContext);
#endif

 private:
  ~VideoCaptureFactory();
};

} 

#endif  
