









#include "webrtc/modules/video_capture/video_capture_impl.h"
#include "webrtc/system_wrappers/interface/ref_count.h"

namespace webrtc {

namespace videocapturemodule {

VideoCaptureModule* VideoCaptureImpl::Create(
    const int32_t id,
    const char* deviceUniqueIdUTF8) {
  RefCountImpl<VideoCaptureImpl>* implementation =
      new RefCountImpl<VideoCaptureImpl>(id);
  return implementation;
}

}  

}  
