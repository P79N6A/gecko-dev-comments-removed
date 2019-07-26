









#include "webrtc/modules/desktop_capture/window_capturer.h"

#include <cassert>

#include "webrtc/modules/desktop_capture/desktop_frame.h"

namespace webrtc {

namespace {

class WindowCapturerMac : public WindowCapturer {
 public:
  WindowCapturerMac();
  virtual ~WindowCapturerMac();

  
  virtual bool GetWindowList(WindowList* windows) OVERRIDE;
  virtual bool SelectWindow(WindowId id) OVERRIDE;

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

 private:
  Callback* callback_;

  DISALLOW_COPY_AND_ASSIGN(WindowCapturerMac);
};

WindowCapturerMac::WindowCapturerMac()
    : callback_(NULL) {
}

WindowCapturerMac::~WindowCapturerMac() {
}

bool WindowCapturerMac::GetWindowList(WindowList* windows) {
  
  return false;
}

bool WindowCapturerMac::SelectWindow(WindowId id) {
  
  return false;
}

void WindowCapturerMac::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

void WindowCapturerMac::Capture(const DesktopRegion& region) {
  
  callback_->OnCaptureCompleted(NULL);
}

}  


WindowCapturer* WindowCapturer::Create() {
  return new WindowCapturerMac();
}

}  
