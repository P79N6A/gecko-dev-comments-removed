









#include "webrtc/modules/desktop_capture/mac/window_list_utils.h"

#include <ApplicationServices/ApplicationServices.h>

#include "webrtc/base/macutils.h"

namespace webrtc {

bool GetWindowList(WindowCapturer::WindowList* windows) {
  
  CFArrayRef window_array = CGWindowListCopyWindowInfo(
      kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
      kCGNullWindowID);
  if (!window_array)
    return false;

  
  
  CFIndex count = CFArrayGetCount(window_array);
  for (CFIndex i = 0; i < count; ++i) {
    CFDictionaryRef window = reinterpret_cast<CFDictionaryRef>(
        CFArrayGetValueAtIndex(window_array, i));
    CFStringRef window_title = reinterpret_cast<CFStringRef>(
        CFDictionaryGetValue(window, kCGWindowName));
    CFNumberRef window_id = reinterpret_cast<CFNumberRef>(
        CFDictionaryGetValue(window, kCGWindowNumber));
    CFNumberRef window_layer = reinterpret_cast<CFNumberRef>(
        CFDictionaryGetValue(window, kCGWindowLayer));
    if (window_title && window_id && window_layer) {
      
      int layer;
      CFNumberGetValue(window_layer, kCFNumberIntType, &layer);
      if (layer != 0)
        continue;

      int id;
      CFNumberGetValue(window_id, kCFNumberIntType, &id);
      WindowCapturer::Window window;
      window.id = id;
      if (!rtc::ToUtf8(window_title, &(window.title)) ||
          window.title.empty()) {
        continue;
      }
      windows->push_back(window);
    }
  }

  CFRelease(window_array);
  return true;
}

}  
