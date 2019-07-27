









#include "webrtc/base/gunit.h"
#include "webrtc/base/x11windowpicker.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/testutils.h"
#include "webrtc/base/windowpicker.h"

#if !defined(WEBRTC_LINUX) || defined(WEBRTC_ANDROID)
#error Only for Linux
#endif

namespace rtc {

TEST(X11WindowPickerTest, TestGetWindowList) {
  MAYBE_SKIP_SCREENCAST_TEST();
  X11WindowPicker window_picker;
  WindowDescriptionList descriptions;
  window_picker.Init();
  window_picker.GetWindowList(&descriptions);
}

TEST(X11WindowPickerTest, TestGetDesktopList) {
  MAYBE_SKIP_SCREENCAST_TEST();
  X11WindowPicker window_picker;
  DesktopDescriptionList descriptions;
  EXPECT_TRUE(window_picker.Init());
  EXPECT_TRUE(window_picker.GetDesktopList(&descriptions));
  EXPECT_TRUE(descriptions.size() > 0);
}

}  
