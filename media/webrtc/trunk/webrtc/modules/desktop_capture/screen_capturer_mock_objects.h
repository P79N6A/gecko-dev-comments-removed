









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURER_MOCK_OBJECTS_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURER_MOCK_OBJECTS_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/modules/desktop_capture/screen_capturer.h"

namespace webrtc {

class MockScreenCapturer : public ScreenCapturer {
 public:
  MockScreenCapturer() {}
  virtual ~MockScreenCapturer() {}

  MOCK_METHOD1(Start, void(Callback* callback));
  MOCK_METHOD1(Capture, void(const DesktopRegion& region));
  MOCK_METHOD1(GetScreenList, bool(ScreenList* screens));
  MOCK_METHOD1(SelectScreen, bool(ScreenId id));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockScreenCapturer);
};

class MockScreenCapturerCallback : public ScreenCapturer::Callback {
 public:
  MockScreenCapturerCallback() {}
  virtual ~MockScreenCapturerCallback() {}

  MOCK_METHOD1(CreateSharedMemory, SharedMemory*(size_t));
  MOCK_METHOD1(OnCaptureCompleted, void(DesktopFrame*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockScreenCapturerCallback);
};

}  

#endif  
