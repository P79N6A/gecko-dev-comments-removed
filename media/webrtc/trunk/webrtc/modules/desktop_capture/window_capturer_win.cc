









#include "webrtc/modules/desktop_capture/window_capturer.h"

#include <assert.h>
#include <windows.h>

#include "webrtc/modules/desktop_capture/desktop_frame_win.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

namespace {

typedef HRESULT (WINAPI *DwmIsCompositionEnabledFunc)(BOOL* enabled);



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

BOOL CALLBACK WindowsEnumerationHandler(HWND hwnd, LPARAM param) {
  assert(IsGUIThread(false));
  WindowCapturer::WindowList* list =
      reinterpret_cast<WindowCapturer::WindowList*>(param);

  
  
  int len = GetWindowTextLength(hwnd);
  HWND owner = GetWindow(hwnd, GW_OWNER);
  LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  if (len == 0 || IsIconic(hwnd) || !IsWindowVisible(hwnd) ||
      (owner && !(exstyle & WS_EX_APPWINDOW))) {
    return TRUE;
  }

  
  const size_t kClassLength = 256;
  WCHAR class_name[kClassLength];
  GetClassName(hwnd, class_name, kClassLength);
  
  
  
  if (wcscmp(class_name, L"Progman") == 0 || wcscmp(class_name, L"Button") == 0)
    return TRUE;

  WindowCapturer::Window window;
  window.id = reinterpret_cast<WindowCapturer::WindowId>(hwnd);

  const size_t kTitleLength = 500;
  WCHAR window_title[kTitleLength];
  
  GetWindowText(hwnd, window_title, kTitleLength);
  window.title = Utf16ToUtf8(window_title);

  
  if (window.title.empty())
    return TRUE;

  list->push_back(window);

  return TRUE;
}

class WindowCapturerWin : public WindowCapturer {
 public:
  WindowCapturerWin();
  virtual ~WindowCapturerWin();

  
  virtual bool GetWindowList(WindowList* windows) OVERRIDE;
  virtual bool SelectWindow(WindowId id) OVERRIDE;
  virtual bool BringSelectedWindowToFront() OVERRIDE;

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

 private:
  bool IsAeroEnabled();

  Callback* callback_;

  
  
  HWND window_;

  
  HMODULE dwmapi_library_;
  DwmIsCompositionEnabledFunc is_composition_enabled_func_;

  DesktopSize previous_size_;

  DISALLOW_COPY_AND_ASSIGN(WindowCapturerWin);
};

WindowCapturerWin::WindowCapturerWin()
    : callback_(NULL),
      window_(NULL) {
  
  dwmapi_library_ = LoadLibrary(L"dwmapi.dll");
  if (dwmapi_library_) {
    is_composition_enabled_func_ =
        reinterpret_cast<DwmIsCompositionEnabledFunc>(
            GetProcAddress(dwmapi_library_, "DwmIsCompositionEnabled"));
    assert(is_composition_enabled_func_);
  } else {
    is_composition_enabled_func_ = NULL;
  }
}

WindowCapturerWin::~WindowCapturerWin() {
  if (dwmapi_library_)
    FreeLibrary(dwmapi_library_);
}

bool WindowCapturerWin::IsAeroEnabled() {
  BOOL result = FALSE;
  if (is_composition_enabled_func_)
    is_composition_enabled_func_(&result);
  return result != FALSE;
}

bool WindowCapturerWin::GetWindowList(WindowList* windows) {
  assert(IsGUIThread(false));
  WindowList result;
  LPARAM param = reinterpret_cast<LPARAM>(&result);
  if (!EnumWindows(&WindowsEnumerationHandler, param))
    return false;
  windows->swap(result);
  return true;
}

bool WindowCapturerWin::SelectWindow(WindowId id) {
  assert(IsGUIThread(false));
  HWND window = reinterpret_cast<HWND>(id);
  if (!IsWindow(window) || !IsWindowVisible(window) || IsIconic(window))
    return false;
  window_ = window;
  previous_size_.set(0, 0);
  return true;
}

bool WindowCapturerWin::BringSelectedWindowToFront() {
  assert(IsGUIThread(false));
  if (!window_)
    return false;

  if (!IsWindow(window_) || !IsWindowVisible(window_) || IsIconic(window_))
    return false;

  return SetForegroundWindow(window_) != 0;
}

void WindowCapturerWin::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

void WindowCapturerWin::Capture(const DesktopRegion& region) {
  assert(IsGUIThread(false));
  if (!window_) {
    LOG(LS_ERROR) << "Window hasn't been selected: " << GetLastError();
    callback_->OnCaptureCompleted(NULL);
    return;
  }

  
  if (IsIconic(window_) || !IsWindowVisible(window_)) {
    callback_->OnCaptureCompleted(NULL);
    return;
  }

  RECT rect;
  if (!GetWindowRect(window_, &rect)) {
    LOG(LS_WARNING) << "Failed to get window size: " << GetLastError();
    callback_->OnCaptureCompleted(NULL);
    return;
  }

  HDC window_dc = GetWindowDC(window_);
  if (!window_dc) {
    LOG(LS_WARNING) << "Failed to get window DC: " << GetLastError();
    callback_->OnCaptureCompleted(NULL);
    return;
  }

  scoped_ptr<DesktopFrameWin> frame(DesktopFrameWin::Create(
      DesktopSize(rect.right - rect.left, rect.bottom - rect.top),
      NULL, window_dc));
  if (!frame.get()) {
    ReleaseDC(window_, window_dc);
    callback_->OnCaptureCompleted(NULL);
    return;
  }

  HDC mem_dc = CreateCompatibleDC(window_dc);
  HGDIOBJ previous_object = SelectObject(mem_dc, frame->bitmap());
  BOOL result = FALSE;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (!IsAeroEnabled() || !previous_size_.equals(frame->size())) {
    result = PrintWindow(window_, mem_dc, 0);
  }

  
  if (!result) {
    result = BitBlt(mem_dc, 0, 0, frame->size().width(), frame->size().height(),
                    window_dc, 0, 0, SRCCOPY);
  }

  SelectObject(mem_dc, previous_object);
  DeleteDC(mem_dc);
  ReleaseDC(window_, window_dc);

  previous_size_ = frame->size();

  if (!result) {
    LOG(LS_ERROR) << "Both PrintWindow() and BitBlt() failed.";
    frame.reset();
  }

  callback_->OnCaptureCompleted(frame.release());
}

}  


WindowCapturer* WindowCapturer::Create(const DesktopCaptureOptions& options) {
  return new WindowCapturerWin();
}

}  
