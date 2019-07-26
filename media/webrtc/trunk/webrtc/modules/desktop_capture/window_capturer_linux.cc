









#include "webrtc/modules/desktop_capture/window_capturer.h"

#include <cassert>

#include "webrtc/modules/desktop_capture/desktop_frame.h"

namespace webrtc {

namespace {

class WindowCapturerLinux : public WindowCapturer {
 public:
  WindowCapturerLinux();
  virtual ~WindowCapturerLinux();

  
  virtual bool GetWindowList(WindowList* windows) OVERRIDE;
  virtual bool SelectWindow(WindowId id) OVERRIDE;

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

 private:
  Callback* callback_;

  DISALLOW_COPY_AND_ASSIGN(WindowCapturerLinux);
};

WindowCapturerLinux::WindowCapturerLinux()
    : callback_(NULL) {
}

WindowCapturerLinux::~WindowCapturerLinux() {
}

bool WindowCapturerLinux::GetWindowList(WindowList* windows) {
  
  return false;
}

bool WindowCapturerLinux::SelectWindow(WindowId id) {
  
  return false;
}

void WindowCapturerLinux::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

void WindowCapturerLinux::Capture(const DesktopRegion& region) {
  
  callback_->OnCaptureCompleted(NULL);
}

}  


WindowCapturer* WindowCapturer::Create() {
  return new WindowCapturerLinux();
}

}  
