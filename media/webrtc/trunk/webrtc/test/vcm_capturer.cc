









#include "webrtc/test/vcm_capturer.h"

#include "webrtc/modules/video_capture/include/video_capture_factory.h"
#include "webrtc/video_send_stream.h"

namespace webrtc {
namespace test {

VcmCapturer::VcmCapturer(webrtc::VideoSendStreamInput* input)
    : VideoCapturer(input), started_(false), vcm_(NULL) {}

bool VcmCapturer::Init(size_t width, size_t height, size_t target_fps) {
  VideoCaptureModule::DeviceInfo* device_info =
      VideoCaptureFactory::CreateDeviceInfo(42);  

  char device_name[256];
  char unique_name[256];
  if (device_info->GetDeviceName(0, device_name, sizeof(device_name),
                                 unique_name, sizeof(unique_name)) !=
      0) {
    Destroy();
    return false;
  }

  vcm_ = webrtc::VideoCaptureFactory::Create(0, unique_name);
  vcm_->RegisterCaptureDataCallback(*this);

  device_info->GetCapability(vcm_->CurrentDeviceName(), 0, capability_);
  delete device_info;

  capability_.width = static_cast<int32_t>(width);
  capability_.height = static_cast<int32_t>(height);
  capability_.maxFPS = static_cast<int32_t>(target_fps);
  capability_.rawType = kVideoI420;

  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return false;
  }

  assert(vcm_->CaptureStarted());

  return true;
}

VcmCapturer* VcmCapturer::Create(VideoSendStreamInput* input,
                                 size_t width, size_t height,
                                 size_t target_fps) {
  VcmCapturer* vcm__capturer = new VcmCapturer(input);
  if (!vcm__capturer->Init(width, height, target_fps)) {
    
    delete vcm__capturer;
    return NULL;
  }
  return vcm__capturer;
}


void VcmCapturer::Start() { started_ = true; }

void VcmCapturer::Stop() { started_ = false; }

void VcmCapturer::Destroy() {
  if (vcm_ == NULL) {
    return;
  }

  vcm_->StopCapture();
  vcm_->DeRegisterCaptureDataCallback();
  vcm_->Release();

  
  
  vcm_ = NULL;
}

VcmCapturer::~VcmCapturer() { Destroy(); }

void VcmCapturer::OnIncomingCapturedFrame(const int32_t id,
                                          I420VideoFrame& frame) {
  if (started_)
    input_->SwapFrame(&frame);
}

void VcmCapturer::OnCaptureDelayChanged(const int32_t id, const int32_t delay) {
}
}  
}  
