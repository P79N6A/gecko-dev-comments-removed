








#ifndef SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_HELPERS_VIE_FAKE_CAMERA_H_
#define SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_HELPERS_VIE_FAKE_CAMERA_H_

#include <string>

namespace webrtc {
class ViECapture;
class ThreadWrapper;
}

class ViEFileCaptureDevice;






class ViEFakeCamera {
 public:
  
  explicit ViEFakeCamera(webrtc::ViECapture* capture_interface);
  virtual ~ViEFakeCamera();

  
  bool StartCameraInNewThread(const std::string& i420_test_video_path,
                              int width,
                              int height);
  
  bool StopCamera();

  int capture_id() const { return capture_id_; }

 private:
  webrtc::ViECapture* capture_interface_;

  int capture_id_;
  webrtc::ThreadWrapper* camera_thread_;
  ViEFileCaptureDevice* file_capture_device_;
};

#endif  
