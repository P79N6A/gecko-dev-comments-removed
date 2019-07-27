








#include "webrtc/modules/desktop_capture/window_capturer.h"
#include "webrtc/modules/desktop_capture/app_capturer.h"

#include <assert.h>

#include "webrtc/modules/desktop_capture/desktop_frame.h"

namespace webrtc {

namespace {

class AppCapturerNull : public AppCapturer {
public:
  AppCapturerNull();
  virtual ~AppCapturerNull();

  
  virtual bool GetAppList(AppList* apps) OVERRIDE;
  virtual bool SelectApp(ProcessId id) OVERRIDE;
  virtual bool BringAppToFront()	OVERRIDE;

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

private:
  Callback* callback_;

  DISALLOW_COPY_AND_ASSIGN(AppCapturerNull);
};

AppCapturerNull::AppCapturerNull()
  : callback_(NULL) {
}

AppCapturerNull::~AppCapturerNull() {
}

bool AppCapturerNull::GetAppList(AppList* apps) {
  
  return false;
}

bool AppCapturerNull::SelectApp(ProcessId id) {
  
  return false;
}

bool AppCapturerNull::BringAppToFront() {
  
  return false;
}


void AppCapturerNull::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

void AppCapturerNull::Capture(const DesktopRegion& region) {
  
  callback_->OnCaptureCompleted(NULL);
}

}  


AppCapturer* AppCapturer::Create(const DesktopCaptureOptions& options) {
  return new AppCapturerNull();
}

}  
