








#include "webrtc/modules/desktop_capture/window_capturer.h"
#include "webrtc/modules/desktop_capture/app_capturer.h"

#include <assert.h>
#include <windows.h>

#include "webrtc/modules/desktop_capture/desktop_frame_win.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

namespace {

std::string Utf16ToUtf8(const WCHAR* str) {
  int len_utf8 = WideCharToMultiByte(CP_UTF8, 0, str, -1,
                                     NULL, 0, NULL, NULL);
  if (len_utf8 <= 0)
    return std::string();
  std::string result(len_utf8, '\0');
  int rv = WideCharToMultiByte(CP_UTF8, 0, str, -1,
                               &*(result.begin()), len_utf8, NULL, NULL);
  if (rv != len_utf8)
    assert(false);

  return result;
}


class AppCapturerWin : public AppCapturer {
public:
  AppCapturerWin();
  virtual ~AppCapturerWin();

  
  virtual bool GetAppList(AppList* apps) OVERRIDE;
  virtual bool SelectApp(ProcessId id) OVERRIDE;
  virtual bool BringAppToFront() OVERRIDE;

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

private:
  Callback* callback_;

  ProcessId processId_;

  DISALLOW_COPY_AND_ASSIGN(AppCapturerWin);
};

AppCapturerWin::AppCapturerWin()
  : callback_(NULL),
    processId_(NULL) {
}

AppCapturerWin::~AppCapturerWin() {
}


bool AppCapturerWin::GetAppList(AppList* apps){
  
  return true;
}

bool AppCapturerWin::SelectApp(ProcessId id) {
  
  return true;
}

bool AppCapturerWin::BringAppToFront() {
  
  return true;
}


void AppCapturerWin::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}
void AppCapturerWin::Capture(const DesktopRegion& region) {
  
}

}  


AppCapturer* AppCapturer::Create(const DesktopCaptureOptions& options) {
  return new AppCapturerWin();
}

}  
