









#include "webrtc/modules/desktop_capture/window_capturer.h"

#include <assert.h>

#include "webrtc/modules/desktop_capture/desktop_frame.h"

namespace webrtc {

namespace {

class WindowCapturerNull : public WindowCapturer {
 public:
  WindowCapturerNull();
  virtual ~WindowCapturerNull();

  
  virtual bool GetWindowList(WindowList* windows) OVERRIDE;
  virtual bool SelectWindow(WindowId id) OVERRIDE;

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

 private:
  Callback* callback_;

  DISALLOW_COPY_AND_ASSIGN(WindowCapturerNull);
};

WindowCapturerNull::WindowCapturerNull()
    : callback_(NULL) {
}

WindowCapturerNull::~WindowCapturerNull() {
}

bool WindowCapturerNull::GetWindowList(WindowList* windows) {
  
  return false;
}

bool WindowCapturerNull::SelectWindow(WindowId id) {
  
  return false;
}

void WindowCapturerNull::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

void WindowCapturerNull::Capture(const DesktopRegion& region) {
  
  callback_->OnCaptureCompleted(NULL);
}

}  


WindowCapturer* WindowCapturer::Create(const DesktopCaptureOptions& options) {
  return new WindowCapturerNull();
}

}  
