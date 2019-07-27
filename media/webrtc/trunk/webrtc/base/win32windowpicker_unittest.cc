








#include "webrtc/base/gunit.h"
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/win32window.h"
#include "webrtc/base/win32windowpicker.h"
#include "webrtc/base/windowpicker.h"

#if !defined(WEBRTC_WIN)
#error Only for Windows
#endif

namespace rtc {

static const TCHAR* kVisibleWindowTitle = L"Visible Window";
static const TCHAR* kInvisibleWindowTitle = L"Invisible Window";

class Win32WindowPickerForTest : public Win32WindowPicker {
 public:
  Win32WindowPickerForTest() {
    EXPECT_TRUE(visible_window_.Create(NULL, kVisibleWindowTitle, WS_VISIBLE,
                                       0, 0, 0, 0, 0));
    EXPECT_TRUE(invisible_window_.Create(NULL, kInvisibleWindowTitle, 0,
                                         0, 0, 0, 0, 0));
  }

  ~Win32WindowPickerForTest() {
    visible_window_.Destroy();
    invisible_window_.Destroy();
  }

  virtual bool GetWindowList(WindowDescriptionList* descriptions) {
    if (!Win32WindowPicker::EnumProc(visible_window_.handle(),
                                     reinterpret_cast<LPARAM>(descriptions))) {
      return false;
    }
    if (!Win32WindowPicker::EnumProc(invisible_window_.handle(),
                                     reinterpret_cast<LPARAM>(descriptions))) {
      return false;
    }
    return true;
  }

  Win32Window* visible_window() {
    return &visible_window_;
  }

  Win32Window* invisible_window() {
    return &invisible_window_;
  }

 private:
  Win32Window visible_window_;
  Win32Window invisible_window_;
};

TEST(Win32WindowPickerTest, TestGetWindowList) {
  Win32WindowPickerForTest window_picker;
  WindowDescriptionList descriptions;
  EXPECT_TRUE(window_picker.GetWindowList(&descriptions));
  EXPECT_EQ(1, descriptions.size());
  WindowDescription desc = descriptions.front();
  EXPECT_EQ(window_picker.visible_window()->handle(), desc.id().id());
  TCHAR window_title[500];
  GetWindowText(window_picker.visible_window()->handle(), window_title,
                ARRAY_SIZE(window_title));
  EXPECT_EQ(0, wcscmp(window_title, kVisibleWindowTitle));
}

TEST(Win32WindowPickerTest, TestIsVisible) {
  Win32WindowPickerForTest window_picker;
  HWND visible_id = window_picker.visible_window()->handle();
  HWND invisible_id = window_picker.invisible_window()->handle();
  EXPECT_TRUE(window_picker.IsVisible(WindowId(visible_id)));
  EXPECT_FALSE(window_picker.IsVisible(WindowId(invisible_id)));
}

TEST(Win32WindowPickerTest, TestMoveToFront) {
  Win32WindowPickerForTest window_picker;
  HWND visible_id = window_picker.visible_window()->handle();
  HWND invisible_id = window_picker.invisible_window()->handle();

  
  
  
  
  window_picker.MoveToFront(WindowId(visible_id));
  window_picker.MoveToFront(WindowId(invisible_id));
}

}  
