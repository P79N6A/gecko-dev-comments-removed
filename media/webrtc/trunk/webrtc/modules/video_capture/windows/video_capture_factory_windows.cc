









#include "ref_count.h"
#include "video_capture_ds.h"
#include "video_capture_mf.h"

namespace webrtc {
namespace videocapturemodule {


VideoCaptureModule::DeviceInfo* VideoCaptureImpl::CreateDeviceInfo(
    const int32_t id) {
  
  return DeviceInfoDS::Create(id);
}

VideoCaptureModule* VideoCaptureImpl::Create(const int32_t id,
                                             const char* device_id) {
  if (device_id == NULL)
    return NULL;

  
  RefCountImpl<VideoCaptureDS>* capture = new RefCountImpl<VideoCaptureDS>(id);
  if (capture->Init(id, device_id) != 0) {
    delete capture;
    capture = NULL;
  }

  return capture;
}

}  
}  
