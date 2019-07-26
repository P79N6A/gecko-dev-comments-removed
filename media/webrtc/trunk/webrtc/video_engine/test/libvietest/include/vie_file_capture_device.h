









#ifndef SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_HELPERS_VIE_FILE_CAPTURE_DEVICE_H_
#define SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_HELPERS_VIE_FILE_CAPTURE_DEVICE_H_

#include <stdio.h>

#include <string>

#include "webrtc/typedefs.h"

namespace webrtc {
class CriticalSectionWrapper;
class EventWrapper;
class ViEExternalCapture;
}



class ViEFileCaptureDevice {
 public:
  
  explicit ViEFileCaptureDevice(webrtc::ViEExternalCapture* input_sink);
  virtual ~ViEFileCaptureDevice();

  
  
  bool OpenI420File(const std::string& path, int width, int height);

  
  
  
  void ReadFileFor(uint64_t time_slice_ms, uint32_t max_fps);

  
  void CloseFile();

 private:
  webrtc::ViEExternalCapture* input_sink_;

  FILE* input_file_;
  webrtc::CriticalSectionWrapper* mutex_;

  uint32_t frame_length_;
  uint32_t width_;
  uint32_t height_;
};

#endif  
