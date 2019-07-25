









#include "video_capture_factory.h"
#include "video_capture_impl.h"
#ifdef WEBRTC_ANDROID
#include "video_capture_android.h"
#endif

namespace webrtc
{

VideoCaptureModule* VideoCaptureFactory::Create(const WebRtc_Word32 id,
    const char* deviceUniqueIdUTF8) {
  return videocapturemodule::VideoCaptureImpl::Create(id, deviceUniqueIdUTF8);
}

VideoCaptureModule* VideoCaptureFactory::Create(const WebRtc_Word32 id,
    VideoCaptureExternal*& externalCapture) {
  return videocapturemodule::VideoCaptureImpl::Create(id, externalCapture);
}

VideoCaptureModule::DeviceInfo* VideoCaptureFactory::CreateDeviceInfo(
    const WebRtc_Word32 id) {
  return videocapturemodule::VideoCaptureImpl::CreateDeviceInfo(id);
}

#ifdef WEBRTC_ANDROID
WebRtc_Word32 VideoCaptureFactory::SetAndroidObjects(void* javaVM,
    void* javaContext) {
  return videocapturemodule::VideoCaptureAndroid::SetAndroidObjects(javaVM,
      javaContext);
}
#endif

} 
