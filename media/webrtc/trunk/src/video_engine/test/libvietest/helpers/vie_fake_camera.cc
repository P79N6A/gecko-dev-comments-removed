








#include "video_engine/test/libvietest/include/vie_fake_camera.h"

#include <assert.h>

#include "system_wrappers/interface/thread_wrapper.h"
#include "video_engine/include/vie_capture.h"
#include "video_engine/test/libvietest/include/vie_file_capture_device.h"


bool StreamVideoFileRepeatedlyIntoCaptureDevice(void* data) {
  ViEFileCaptureDevice* file_capture_device =
      reinterpret_cast<ViEFileCaptureDevice*>(data);

  
  
  
  uint64_t time_slice_ms = 1500;
  uint32_t max_fps = 30;

  file_capture_device->ReadFileFor(time_slice_ms, max_fps);

  return true;
}

ViEFakeCamera::ViEFakeCamera(webrtc::ViECapture* capture_interface)
    : capture_interface_(capture_interface),
      capture_id_(-1),
      camera_thread_(NULL),
      file_capture_device_(NULL) {
}

ViEFakeCamera::~ViEFakeCamera() {
}

bool ViEFakeCamera::StartCameraInNewThread(
    const std::string& i420_test_video_path, int width, int height) {

  assert(file_capture_device_ == NULL && camera_thread_ == NULL);

  webrtc::ViEExternalCapture* externalCapture;
  int result = capture_interface_->
      AllocateExternalCaptureDevice(capture_id_, externalCapture);
  if (result != 0) {
    return false;
  }

  file_capture_device_ = new ViEFileCaptureDevice(externalCapture);
  if (!file_capture_device_->OpenI420File(i420_test_video_path,
                                          width,
                                          height)) {
    return false;
  }

  
  
  camera_thread_ = webrtc::ThreadWrapper::CreateThread(
      StreamVideoFileRepeatedlyIntoCaptureDevice, file_capture_device_);
  unsigned int id;
  camera_thread_->Start(id);

  return true;
}

bool ViEFakeCamera::StopCamera() {
  assert(file_capture_device_ != NULL && camera_thread_ != NULL);

  camera_thread_->Stop();
  file_capture_device_->CloseFile();

  int result = capture_interface_->ReleaseCaptureDevice(capture_id_);

  delete camera_thread_;
  delete file_capture_device_;
  camera_thread_ = NULL;
  file_capture_device_ = NULL;

  return result == 0;
}
