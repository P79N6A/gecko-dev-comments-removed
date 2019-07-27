








#include "webrtc/base/gunit.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/macutils.h"
#include "webrtc/base/macwindowpicker.h"
#include "webrtc/base/windowpicker.h"

#if !defined(WEBRTC_MAC) || defined(WEBRTC_IOS)
#error Only for WEBRTC_MAC && !WEBRTC_IOS
#endif

namespace rtc {

bool IsLeopardOrLater() {
  return GetOSVersionName() >= kMacOSLeopard;
}


TEST(MacWindowPickerTest, TestGetWindowList) {
  MacWindowPicker picker, picker2;
  WindowDescriptionList descriptions;
  if (IsLeopardOrLater()) {
    EXPECT_TRUE(picker.Init());
    EXPECT_TRUE(picker.GetWindowList(&descriptions));
    EXPECT_TRUE(picker2.GetWindowList(&descriptions));  
  } else {
    EXPECT_FALSE(picker.Init());
    EXPECT_FALSE(picker.GetWindowList(&descriptions));
    EXPECT_FALSE(picker2.GetWindowList(&descriptions));
  }
}





}  
