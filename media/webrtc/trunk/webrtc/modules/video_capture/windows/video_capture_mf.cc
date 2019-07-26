









#include "modules/video_capture/windows/video_capture_mf.h"

namespace webrtc {
namespace videocapturemodule {

VideoCaptureMF::VideoCaptureMF(const WebRtc_Word32 id) : VideoCaptureImpl(id) {}
VideoCaptureMF::~VideoCaptureMF() {}

WebRtc_Word32 VideoCaptureMF::Init(const WebRtc_Word32 id,
                                   const char* device_id) {
  return 0;
}

WebRtc_Word32 VideoCaptureMF::StartCapture(
    const VideoCaptureCapability& capability) {
  return -1;
}

WebRtc_Word32 VideoCaptureMF::StopCapture() {
  return -1;
}

bool VideoCaptureMF::CaptureStarted() {
  return false;
}

WebRtc_Word32 VideoCaptureMF::CaptureSettings(
    VideoCaptureCapability& settings) {
  return -1;
}

}  
}  
